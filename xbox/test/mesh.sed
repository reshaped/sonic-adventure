# _mesh_release tells the list's marker, the list's address negated, from a
# mesh's address by its sign: on the console every address of RAM is
# negative as a signed number, so a prev of 0 or more is the marker.
# qemu-sh4 maps no address past 0x7fffffff, so under it every address is
# positive and the marker negative. the rewrite asks the same question for
# that layout, whether prev's sign is a mesh's opposite: T takes R3's sign
# bit, by SHLL of a copy in R0, which _mesh_release writes before it reads
# again. qemu-sh4's ROTL leaves T clear where the bit is set
s/^\(\s*\)CMP\/PZ\(\s*\)R3\(\s*\)! 0 or more: the list's, negated/\1MOV         R3,R0\n\1SHLL        R0                      ! qemu: negative is the list's marker/
