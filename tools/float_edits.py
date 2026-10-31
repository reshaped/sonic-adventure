"""Edits of GCC's preprocessed C by token span, for tools/floats.py, and the
C they give.

An edit writes its pieces in the place of a span of tokens. Each piece is
a text, or a span of tokens inside the edit's, which it writes as the C
writes those tokens, on one line, with the edits inside that span. A
piece's span can be the edit's whole span, and pieces can repeat a span,
as a wrap that copies its operand twice does. No two edits take one span,
and an edit lies inside another only inside one of the other's piece
spans, so each token has one owner; text() fails on any other overlap. An
edit's text takes the place of its first token and its other tokens give
nothing, so what lies between them, line markers and newlines among it,
stays, and each line keeps its number. Every other token gives its text.
"""

from float_tokens import quoted


class Plan:
    """The edits of toks, GCC's preprocessed C as marked() gives it, each by
    the index of its first token and the index past its last."""

    def __init__(self, toks):
        self.toks, self.edits, self.ends = toks, {}, {}

    def edit(self, start, end, pieces):
        """Write pieces in the place of toks[start:end]: each a text, or a
        (start, end) span of the tokens inside it. A ValueError says where an
        edit takes the span already."""
        if (start, end) in self.edits:
            raise ValueError(self.overlap(start, end))
        self.edits[start, end] = pieces
        self.ends.setdefault(start, []).append(end)

    def overlap(self, start, end):
        return f'{self.toks.where(start)}: {quoted(self.toks[start:end])}: two of the tool\'s edits overlap here'

    def written(self, start, end, outer=None):
        """toks[start:end] on one line, as quoted() gives them, with each edit
        inside them in the place of its tokens: at each token, the longest
        edit that starts there. outer, the edit whose piece toks[start:end]
        is, stays out, so a piece that copies its edit's whole span writes
        the tokens, not the edit again."""
        pieces, at = [], start
        while at < end:
            stop = max((past for past in self.ends.get(at, ()) if past <= end and (at, past) != outer), default=None)
            if stop is None:
                pieces.append(self.toks[at].text)
                stop = at + 1
            else:
                pieces += [piece if isinstance(piece, str) else self.written(*piece, (at, stop))
                           for piece in self.edits[at, stop]]
            if stop < end and self.toks[stop].start > self.toks[stop - 1].end:
                pieces.append(' ')
            at = stop
        return ''.join(pieces)

    def text(self, source):
        """source, the C toks hold, with each edit that lies inside no other
        in the place of its tokens. A ValueError says where two edits
        overlap otherwise than one inside a span of the other."""
        outermost, open_ = {}, []
        for start, end in sorted(self.edits, key=lambda span: (span[0], -span[1])):
            while open_ and open_[-1][1] <= start:
                open_.pop()
            if open_ and not any(not isinstance(piece, str) and piece[0] <= start and end <= piece[1]
                                 for piece in self.edits[open_[-1]]):
                raise ValueError(self.overlap(start, end))
            if not open_:
                outermost[start] = end
            open_.append((start, end))
        pieces, kept, past = [], 0, 0
        for at, token in enumerate(self.toks):
            pieces.append(source[kept:token.start])
            if at in outermost:
                past = outermost[at]
                pieces.append(self.written(at, past))
            elif at >= past:
                pieces.append(token.text)
            kept = token.end
        return ''.join(pieces) + source[kept:]
