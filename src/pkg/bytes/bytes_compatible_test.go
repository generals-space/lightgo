package bytes_test

import (
	. "bytes"
	"testing"
)

// 	@compatible: 该函数在 v1.5 版本初次添加
//
func TestLastIndexByte(t *testing.T) {
	testCases := []BinOpTest{
		{"", "q", -1},
		{"abcdef", "q", -1},
		{"abcdefabcdef", "a", len("abcdef")},      // something in the middle
		{"abcdefabcdef", "f", len("abcdefabcde")}, // last byte
		{"zabcdefabcdef", "z", 0},                 // first byte
		{"a☺b☻c☹d", "b", len("a☺")},               // non-ascii
	}
	for _, test := range testCases {
		actual := LastIndexByte([]byte(test.a), test.b[0])
		if actual != test.i {
			t.Errorf("LastIndexByte(%q,%c) = %v; want %v", test.a, test.b[0], actual, test.i)
		}
	}
}
