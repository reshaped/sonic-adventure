-- Flycast's half of the frame comparison with the PC port (tools/flycast/compare.sh).
--
-- Flycast runs this as flycast.lua from its own folder, with compare.sh's
-- plan.txt beside it in compare\. The plan's pads= and shots= lines are the
-- port's HOST_PADS and HOST_SHOT under HOST_CLOCK=frames, so both count
-- Ninja's _nj_vsync_count_, the frames the game has rendered:
--
--   pads=600:8,610:0,2400:0/128/0   port A's buttons, PDD_DGT_ bits in hex, from
--                                   each frame on, and optionally the stick's x/y
--   pokes=300:0x8c793fb0:1:1        HOST_POKE's, with each symbol as its console
--                                   address (compare.sh looks them up): at
--                                   frame 300, the byte there becomes 1
--   shots=900,2700                  frames to capture
--   window=3                        also capture this many frames either side
--   quit=3000                       close Flycast at this frame
--   drive=26                        the frames the port spends on the memory
--                                   card screen it never shows
--   watch=0x8c612524,0x8c61252c     words to log as they change, by blank and
--                                   frame, as HOST_WATCH logs one
--
-- The port skips the memory card screen (XBOX.md's out of scope). Here, while
-- that screen is up, this script holds its clock, presses A until the screen
-- goes, which picks card A1 as the port does, and then counts on as though
-- the screen had lasted the port's drive= frames. The plan's frames stay the
-- port's after it.
--
-- A shot is the displayed framebuffer, read from VRAM at the blank the start
-- address changes, so it needs rend.EmulateFramebuffer. Each is written as
-- compare\fNNNNN.fb: a text line "FB depth words_per_line lines stride ctrl",
-- then the lines. The log, compare\timeline.txt, has each game_mode change
-- and each capture by blank and frame.
--
-- Addresses are 1ST_READ.BIN's, from config/1st_read/symbols.txt.
local GAME_MODE, VSYNC_COUNT = 0x8c7694d0, 0x8c8c2758
-- ADVERTISE's menus: advertise_task, the screen at +4 of its extra at +44
local ADVERTISE_TASK, SCREEN_DRIVE = 0x8c941fb0, 4
-- compare.sh and sound.sh put COMPARE_DIR, FLYCAST_DIR's compare\, first
local DIR = assert(COMPARE_DIR, "COMPARE_DIR: run compare.lua through compare.sh or sound.sh")

local rd32 = flycast.memory.read32
local plan = { pads = {}, pokes = {}, shots = {}, window = 0, quit = nil, drive = 26, watch = {} }
-- the clock: frames, less the frames the memory card screen took past the
-- port's; drive_at is the clock while the screen is up
local offset, drive_at, drive_frame = 0, nil, 0
local blank, last_mode, last_sof, log = 0, nil, nil, nil
local wanted, done, next_pad = {}, {}, 1

local function read_plan()
  local f = io.open(DIR .. "plan.txt", "r")
  if f == nil then return end
  for line in f:lines() do
    local key, value = line:match("^(%w+)=(.*)$")
    if key == "pads" then
      for from, buttons, rest in value:gmatch("(%d+):(%x+)([/%d]*)") do
        local x, y = rest:match("^/(%d+)/(%d+)$")
        plan.pads[#plan.pads + 1] = { from = tonumber(from), buttons = tonumber(buttons, 16),
                                      x = tonumber(x) or 128, y = tonumber(y) or 128 }
      end
    elseif key == "pokes" then
      for at, address, value, size in value:gmatch("(%d+):(0x%x+):(%w+):?(%d*)") do
        plan.pokes[#plan.pokes + 1] = { at = tonumber(at), address = tonumber(address),
                                        value = tonumber(value), size = tonumber(size) or 4 }
      end
    elseif key == "shots" then
      for at in value:gmatch("%d+") do plan.shots[#plan.shots + 1] = tonumber(at) end
    elseif key == "window" then
      plan.window = tonumber(value)
    elseif key == "quit" then
      plan.quit = tonumber(value)
    elseif key == "drive" then
      plan.drive = tonumber(value)
    elseif key == "watch" then
      for address in value:gmatch("0x%x+") do plan.watch[#plan.watch + 1] = { address = tonumber(address) } end
    end
  end
  f:close()
  for _, at in ipairs(plan.shots) do
    for k = at - plan.window, at + plan.window do wanted[k] = true end
  end
end

local function dump(name)
  local ctrl, sof, size = rd32(0xa05f8044), rd32(0xa05f8050), rd32(0xa05f805c)
  local depth = (ctrl >> 2) & 3              -- 0 0555, 1 565, 2 888, 3 0888
  local xw = (size & 0x3ff) + 1              -- 32-bit words per line
  local h = ((size >> 10) & 0x3ff) + 1
  local stride = xw + ((size >> 20) & 0x3ff) - 1
  local f = io.open(DIR .. name .. ".fb", "wb")
  f:write(string.format("FB %d %d %d %d 0x%08x\n", depth, xw, h, stride, ctrl))
  local base = 0xa5000000 + (sof & 0xfffffc)
  local pack = string.pack
  for y = 0, h - 1 do
    local row, a = {}, base + y * stride * 4
    for x = 0, xw - 1 do row[x + 1] = pack("<I4", rd32(a + x * 4)) end
    f:write(table.concat(row))
  end
  f:close()
end

local function in_ram(a) return a >= 0x8c000000 and a < 0x8d000000 end

-- the menus' screen, or nil outside the menus
local function screen()
  local task = rd32(ADVERTISE_TASK)
  if not in_ram(task) then return nil end
  local extra = rd32(task + 44)
  if not in_ram(extra) then return nil end
  return rd32(extra + 4)
end

-- port A, as HOST_PADS sets it at frame frames
local function set_pads(frames)
  local pad = nil
  while next_pad <= #plan.pads and plan.pads[next_pad].from <= frames do next_pad = next_pad + 1 end
  if next_pad > 1 then pad = plan.pads[next_pad - 1] end
  if pad == nil then return end
  flycast.input.releaseButtons(1, 0xffff)
  flycast.input.pressButtons(1, pad.buttons)
  flycast.input.setAxis(1, 1, (pad.x << 8) - 32768)
  flycast.input.setAxis(1, 2, (pad.y << 8) - 32768)
end

flycast_callbacks = {
  start = function()
    read_plan()
    log = io.open(DIR .. "timeline.txt", "w")
    blank, last_mode, last_sof, next_pad = 0, nil, nil, 1
    offset, drive_at = 0, nil
  end,
  vblank = function()
    if log == nil then return end
    blank = blank + 1
    local mode, raw, sof = rd32(GAME_MODE), rd32(VSYNC_COUNT), rd32(0xa05f8050)
    -- the memory card screen: hold the clock, and pulse A until it goes
    if mode == 11 and screen() == SCREEN_DRIVE then
      if drive_at == nil then
        drive_at, drive_frame = raw - offset, raw
        log:write(string.format("blank %d frame %d memory card screen\n", blank, raw)); log:flush()
      end
      flycast.input.releaseButtons(1, 0xffff)
      if (raw - drive_frame) % 8 >= 4 then flycast.input.pressButtons(1, 4) end
      return
    elseif drive_at ~= nil then
      offset = raw - (drive_at + plan.drive)
      log:write(string.format("blank %d frame %d memory card screen gone: clock %d\n", blank, raw, raw - offset))
      drive_at = nil
    end
    local frames = raw - offset
    if mode ~= last_mode then
      log:write(string.format("blank %d frame %d game_mode %d\n", blank, frames, mode)); log:flush()
      last_mode = mode
    end
    -- a new picture shows: the one the game rendered last
    if sof ~= last_sof then
      last_sof = sof
      if wanted[frames] and not done[frames] then
        done[frames] = true
        dump(string.format("f%05d", frames))
        log:write(string.format("blank %d frame %d shot\n", blank, frames)); log:flush()
      end
    end
    for _, w in ipairs(plan.watch) do
      local v = rd32(w.address)
      if v ~= w.value then
        w.value = v
        log:write(string.format("blank %d frame %d watch 0x%08x = %d\n", blank, frames, w.address, v))
      end
    end
    set_pads(frames)
    for _, poke in ipairs(plan.pokes) do
      if not poke.done and frames >= poke.at then
        poke.done = true
        local write = poke.size == 1 and flycast.memory.write8 or poke.size == 2 and flycast.memory.write16
                      or flycast.memory.write32
        write(poke.address, poke.value)
        log:write(string.format("blank %d frame %d poke 0x%08x = %d\n", blank, frames, poke.address, poke.value))
      end
    end
    if plan.quit ~= nil and frames >= plan.quit then
      log:write(string.format("blank %d frame %d quit\n", blank, frames)); log:close(); log = nil
      flycast.emulator.exit()
    end
  end,
}
