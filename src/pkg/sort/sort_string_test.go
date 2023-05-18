// 文件名必须是 _test.go 结尾

// 注意包名, 与所在包不同.

package sort_test

import (
	"testing"

	"sort"
)

var stringlist []string = []string {
	"hello world",
}

var expected []string = []string {
	" dehllloorw",
}

////////////////////////////////////////////////////////////////////////////////
// 单元测试(测试函数必须以 Test 开头, 且为大驼峰)
//
// 执行方法: go test -v sort_string_test.go

func TestString(t *testing.T) {
	for i, str := range stringlist {
		actual := sort.String(str)
		if actual != expected[i] {
			t.Errorf("wrong answer for %s: %s", str, actual)
		}
	}
}

func TestString2(t *testing.T) {
	for i, str := range stringlist {
		actual := sort.String2(str)
		if actual != expected[i] {
			t.Errorf("wrong answer for %s: %s", str, actual)
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// 压力测试(测试函数必须以 Benchmark 开头, 且为大驼峰)
//
// 执行方法: go test sort_string_test.go -test.bench=".*"

func BenchmarkString(b *testing.B) {
	// 调用该函数停止压力测试的时间计数
    b.StopTimer()

    // 做一些初始化的工作,例如读取文件数据,数据库连接之类的,
    // 这样这些时间不影响我们测试函数本身的性能

	// 重新开始时间
    b.StartTimer()

	for i := 0; i < b.N; i ++ {
		for _, str := range stringlist {
			sort.String(str)
		}
	}
}

func BenchmarkString2(b *testing.B) {
	for i := 0; i < b.N; i ++ {
		for _, str := range stringlist {
			sort.String2(str)
		}
	}
}
