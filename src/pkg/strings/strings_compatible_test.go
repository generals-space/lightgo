package strings_test

import (
	"testing"
	. "strings"
)

func TestLastIndexByte(t *testing.T) {
	testCases := []IndexTest{
		{"", "q", -1},
		{"abcdef", "q", -1},
		{"abcdefabcdef", "a", len("abcdef")},      // something in the middle
		{"abcdefabcdef", "f", len("abcdefabcde")}, // last byte
		{"zabcdefabcdef", "z", 0},                 // first byte
		{"a☺b☻c☹d", "b", len("a☺")},               // non-ascii
	}
	for _, test := range testCases {
		actual := LastIndexByte(test.s, test.sep[0])
		if actual != test.out {
			t.Errorf("LastIndexByte(%q,%c) = %v; want %v", test.s, test.sep[0], actual, test.out)
		}
	}
}
