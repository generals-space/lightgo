// Copyright 2017 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package strings_test

import (
	"bytes"
	"runtime"
	. "strings"
	"testing"
)

// 	@todo: testing 包中缺少 Helper() 方法, 后续补充
func check(t *testing.T, b *Builder, want string) {
	// 这一行先注释掉
	// t.Helper()
	got := b.String()
	if got != want {
		t.Errorf("String: got %#q; want %#q", got, want)
		return
	}
	if n := b.Len(); n != len(got) {
		t.Errorf("Len: got %d; but len(String()) is %d", n, len(got))
	}
}

func TestBuilder(t *testing.T) {
	var b Builder
	check(t, &b, "")
	n, err := b.WriteString("hello")
	if err != nil || n != 5 {
		t.Errorf("WriteString: got %d,%s; want 5,nil", n, err)
	}
	check(t, &b, "hello")
	if err = b.WriteByte(' '); err != nil {
		t.Errorf("WriteByte: %s", err)
	}
	check(t, &b, "hello ")
	n, err = b.WriteString("world")
	if err != nil || n != 5 {
		t.Errorf("WriteString: got %d,%s; want 5,nil", n, err)
	}
	check(t, &b, "hello world")
}

func TestBuilderString(t *testing.T) {
	var b Builder
	b.WriteString("alpha")
	check(t, &b, "alpha")
	s1 := b.String()
	b.WriteString("beta")
	check(t, &b, "alphabeta")
	s2 := b.String()
	b.WriteString("gamma")
	check(t, &b, "alphabetagamma")
	s3 := b.String()

	// Check that subsequent operations didn't change the returned strings.
	if want := "alpha"; s1 != want {
		t.Errorf("first String result is now %q; want %q", s1, want)
	}
	if want := "alphabeta"; s2 != want {
		t.Errorf("second String result is now %q; want %q", s2, want)
	}
	if want := "alphabetagamma"; s3 != want {
		t.Errorf("third String result is now %q; want %q", s3, want)
	}
}

func TestBuilderReset(t *testing.T) {
	var b Builder
	check(t, &b, "")
	b.WriteString("aaa")
	s := b.String()
	check(t, &b, "aaa")
	b.Reset()
	check(t, &b, "")

	// Ensure that writing after Reset doesn't alter
	// previously returned strings.
	b.WriteString("bbb")
	check(t, &b, "bbb")
	if want := "aaa"; s != want {
		t.Errorf("previous String result changed after Reset: got %q; want %q", s, want)
	}
}

func TestBuilderGrow(t *testing.T) {
	for _, growLen := range []int{0, 100, 1000, 10000, 100000} {
		var b Builder
		b.Grow(growLen)
		p := bytes.Repeat([]byte{'a'}, growLen)
		allocs := numAllocs(func() { b.Write(p) })
		if allocs > 0 {
			t.Errorf("growLen=%d: allocation occurred during write", growLen)
		}
		if b.String() != string(p) {
			t.Errorf("growLen=%d: bad data written after Grow", growLen)
		}
	}
}

func TestBuilderWrite2(t *testing.T) {
	const s0 = "hello 世界"
	for _, tt := range []struct {
		name string
		fn   func(b *Builder) (int, error)
		n    int
		want string
	}{
		{
			"Write",
			func(b *Builder) (int, error) { return b.Write([]byte(s0)) },
			len(s0),
			s0,
		},
		{
			"WriteRune",
			func(b *Builder) (int, error) { return b.WriteRune('a') },
			1,
			"a",
		},
		{
			"WriteRuneWide",
			func(b *Builder) (int, error) { return b.WriteRune('世') },
			3,
			"世",
		},
		{
			"WriteString",
			func(b *Builder) (int, error) { return b.WriteString(s0) },
			len(s0),
			s0,
		},
	} {
		t.Run(tt.name, func(t *testing.T) {
			var b Builder
			n, err := tt.fn(&b)
			if err != nil {
				t.Fatalf("first call: got %s", err)
			}
			if n != tt.n {
				t.Errorf("first call: got n=%d; want %d", n, tt.n)
			}
			check(t, &b, tt.want)

			n, err = tt.fn(&b)
			if err != nil {
				t.Fatalf("second call: got %s", err)
			}
			if n != tt.n {
				t.Errorf("second call: got n=%d; want %d", n, tt.n)
			}
			check(t, &b, tt.want+tt.want)
		})
	}
}

func TestBuilderWriteByte(t *testing.T) {
	var b Builder
	if err := b.WriteByte('a'); err != nil {
		t.Error(err)
	}
	if err := b.WriteByte(0); err != nil {
		t.Error(err)
	}
	check(t, &b, "a\x00")
}

// 	@todo: 测试未通过
func TestBuilderAllocs(t *testing.T) {
	var b Builder
	b.Grow(5)
	var s string
	allocs := numAllocs(func() {
		b.WriteString("hello")
		s = b.String()
	})
	if want := "hello"; s != want {
		t.Errorf("String: got %#q; want %#q", s, want)
	}
	if allocs > 0 {
		t.Fatalf("got %d alloc(s); want 0", allocs)
	}

	// Issue 23382; verify that copyCheck doesn't force the
	// Builder to escape and be heap allocated.
	n := testing.AllocsPerRun(10000, func() {
		var b Builder
		b.Grow(5)
		b.WriteString("abcde")
		_ = b.String()
	})
	if n != 1 {
		t.Errorf("Builder allocs = %v; want 1", n)
	}
}

func numAllocs(fn func()) uint64 {
	defer runtime.GOMAXPROCS(runtime.GOMAXPROCS(1))
	var m1, m2 runtime.MemStats
	runtime.ReadMemStats(&m1)
	fn()
	runtime.ReadMemStats(&m2)
	return m2.Mallocs - m1.Mallocs
}

func TestBuilderCopyPanic(t *testing.T) {
	tests := []struct {
		name      string
		fn        func()
		wantPanic bool
	}{
		{
			name:      "String",
			wantPanic: false,
			fn: func() {
				var a Builder
				a.WriteByte('x')
				b := a
				_ = b.String() // appease vet
			},
		},
		{
			name:      "Len",
			wantPanic: false,
			fn: func() {
				var a Builder
				a.WriteByte('x')
				b := a
				b.Len()
			},
		},
		{
			name:      "Reset",
			wantPanic: false,
			fn: func() {
				var a Builder
				a.WriteByte('x')
				b := a
				b.Reset()
				b.WriteByte('y')
			},
		},
		{
			name:      "Write",
			wantPanic: true,
			fn: func() {
				var a Builder
				a.Write([]byte("x"))
				b := a
				b.Write([]byte("y"))
			},
		},
		{
			name:      "WriteByte",
			wantPanic: true,
			fn: func() {
				var a Builder
				a.WriteByte('x')
				b := a
				b.WriteByte('y')
			},
		},
		{
			name:      "WriteString",
			wantPanic: true,
			fn: func() {
				var a Builder
				a.WriteString("x")
				b := a
				b.WriteString("y")
			},
		},
		{
			name:      "WriteRune",
			wantPanic: true,
			fn: func() {
				var a Builder
				a.WriteRune('x')
				b := a
				b.WriteRune('y')
			},
		},
		{
			name:      "Grow",
			wantPanic: true,
			fn: func() {
				var a Builder
				a.Grow(1)
				b := a
				b.Grow(2)
			},
		},
	}
	for _, tt := range tests {
		didPanic := make(chan bool)
		go func() {
			defer func() { didPanic <- recover() != nil }()
			tt.fn()
		}()
		if got := <-didPanic; got != tt.wantPanic {
			t.Errorf("%s: panicked = %v; want %v", tt.name, got, tt.wantPanic)
		}
	}
}
