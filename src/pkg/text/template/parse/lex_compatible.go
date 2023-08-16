package parse

import "strings"

// 	@compatible: addAt v1.6
//
// Trimming spaces.
// If the action begins "{{- " rather than "{{", then all space/tab/newlines
// preceding the action are trimmed; conversely if it ends " -}}" the
// leading spaces are trimmed. This is done entirely in the lexer; the
// parser never sees it happen. We require an ASCII space to be
// present to avoid ambiguity with things like "{{-3}}". It reads
// better with the space present anyway. For simplicity, only ASCII
// space does the job.
const (
	spaceChars      = " \t\r\n" // These are the space characters defined by Go itself.
	leftTrimMarker  = "- "      // Attached to left delimiter, trims trailing spaces from preceding text.
	rightTrimMarker = " -"      // Attached to right delimiter, trims leading spaces from following text.
	trimMarkerLen   = Pos(len(leftTrimMarker))
)

// atLeftDelim reports whether the lexer is at a left delimiter, possibly followed by a trim marker.
func (l *lexer) atLeftDelim() (delim, trimSpaces bool) {
	if !strings.HasPrefix(l.input[l.pos:], l.leftDelim) {
		return false, false
	}
	// The left delim might have the marker afterwards.
	trimSpaces = strings.HasPrefix(l.input[l.pos+Pos(len(l.leftDelim)):], leftTrimMarker)
	return true, trimSpaces
}

// rightTrimLength returns the length of the spaces at the end of the string.
func rightTrimLength(s string) Pos {
	return Pos(len(s) - len(strings.TrimRight(s, spaceChars)))
}

// atRightDelim reports whether the lexer is at a right delimiter, possibly preceded by a trim marker.
func (l *lexer) atRightDelim() (delim, trimSpaces bool) {
	if strings.HasPrefix(l.input[l.pos:], l.rightDelim) {
		return true, false
	}
	// The right delim might have the marker before.
	if strings.HasPrefix(l.input[l.pos:], rightTrimMarker) {
		if strings.HasPrefix(l.input[l.pos+trimMarkerLen:], l.rightDelim) {
			return true, true
		}
	}
	return false, false
}

// leftTrimLength returns the length of the spaces at the beginning of the string.
func leftTrimLength(s string) Pos {
	return Pos(len(s) - len(strings.TrimLeft(s, spaceChars)))
}

////////////////////////////////////////////////////////////////////////////////
// 	@compatibleNote: 在 v1.6 版本添加了 "{{-" 和 "-}}", 用于抹除两侧的 \n 换行符.
//
// lexLeftDelim scans the left delimiter, which is known to be present, possibly with a trim marker.
func lexLeftDelim(l *lexer) stateFn {
	l.pos += Pos(len(l.leftDelim))
	trimSpace := strings.HasPrefix(l.input[l.pos:], leftTrimMarker)
	afterMarker := Pos(0)
	if trimSpace {
		afterMarker = trimMarkerLen
	}
	if strings.HasPrefix(l.input[l.pos+afterMarker:], leftComment) {
		l.pos += afterMarker
		l.ignore()
		return lexComment
	}
	l.emit(itemLeftDelim)
	l.pos += afterMarker
	l.ignore()
	l.parenDepth = 0
	return lexInsideAction
}

// 	@compatibleNote: 在 v1.6 版本添加了 "{{-" 和 "-}}", 用于抹除两侧的 \n 换行符.
//
// lexRightDelim scans the right delimiter, which is known to be present, possibly with a trim marker.
func lexRightDelim(l *lexer) stateFn {
	trimSpace := strings.HasPrefix(l.input[l.pos:], rightTrimMarker)
	if trimSpace {
		l.pos += trimMarkerLen
		l.ignore()
	}
	l.pos += Pos(len(l.rightDelim))
	l.emit(itemRightDelim)
	if trimSpace {
		l.pos += leftTrimLength(l.input[l.pos:])
		l.ignore()
	}
	return lexText
}
