package time_test

import (
	"fmt"
	"time"
)

////////////////////////////////////////////////////////////////////////////////
// 日期对象 -> 时间戳, 格式化字符串

func ExampleTimeToOthers() {
	// 展示为上海时区
	loc, _ := time.LoadLocation("Asia/Shanghai")
	// 时间戳转Time对象
	var timeStamp int64 = 1525788126
	timeObj := time.Unix(timeStamp, 0).In(loc)

	// Time对象 -> 时间戳
	timeStamp = timeObj.Unix()
	fmt.Printf("%d\n", timeStamp)

	// Time对象 -> 字符串
	//
	// 这种格式化真的是奇葩, 其他语言中的"%Y-%M-%d %H:%m:%S"
	// 在go语言中对应的是"2006-01-02 15:04:05"
	// 这个时间点是go诞生之日, 记忆方法:6-1-2-3-4-5.
	// 可以把2006当作%Y来用, 比如"2006xx2006"会输出: 当前年份xx当前年份...
	//
	var timeStr string
	timeStr = timeObj.Format("2006-01-02 15:04:05")
	fmt.Printf("%s\n", timeStr)
	timeStr = timeObj.Format("2006xx2006")
	fmt.Printf("%s\n", timeStr)

	// Output:
	//
	// 1525788126
	// 2018-05-08 22:02:06
	// 2018xx2018
}

func ExampleTimeToOthers1() {
	// 展示为上海时区
	loc, _ := time.LoadLocation("Asia/Shanghai")
	// 时间戳转Time对象
	var timeStamp int64 = 1525788126
	timeObj := time.Unix(timeStamp, 0).In(loc)

	// 除了`2006-01-02 15:04:05`这些特殊的占位符外, 还有`-07:00`或`Z07:00`(或`-0700`, `Z0700`), 它表示时区.
	// 中间的字符 T 倒不是关键字, 随便写.
	fmt1 := "2006-01-02T15:04:05"
	fmt2 := "2006-01-02T15:04:05-07:00"
	fmt3 := "2006-01-02T15:04:05 -07:00"
	fmt4 := "2006-01-02T15:04:05 -0700"
	fmt5 := "2006-01-02T15:04:05Z07:00"
	fmt6 := "2006-01-02T15:04:05 Z07:00"
	fmt7 := "2006-01-02T15:04:05 Z0700"

	fmt.Println(timeObj.Format(fmt1)) // 2018-06-15T17:02:08
	fmt.Println(timeObj.Format(fmt2)) // 2018-06-15T17:02:08+08:00
	fmt.Println(timeObj.Format(fmt3)) // 2018-06-15T17:02:08 +08:00
	fmt.Println(timeObj.Format(fmt4)) // 2018-06-15T17:02:08 +0800
	fmt.Println(timeObj.Format(fmt5)) // 2018-06-15T17:02:08+08:00
	fmt.Println(timeObj.Format(fmt6)) // 2018-06-15T17:02:08 +08:00
	fmt.Println(timeObj.Format(fmt7)) // 2018-06-15T17:02:08 +0800

	// Output:
	// 2018-05-08T22:02:06
	// 2018-05-08T22:02:06+08:00
	// 2018-05-08T22:02:06 +08:00
	// 2018-05-08T22:02:06 +0800
	// 2018-05-08T22:02:06+08:00
	// 2018-05-08T22:02:06 +08:00
	// 2018-05-08T22:02:06 +0800
}

////////////////////////////////////////////////////////////////////////////////
// 时间戳/字符串 -> 日期对象

func ExampleOthersToTime() {
	// 展示为上海时区
	loc, _ := time.LoadLocation("Asia/Shanghai")
	// 时间戳转Time对象
	var timeStamp int64 = 1525788126
	timeObj := time.Unix(timeStamp, 0).In(loc)

	timeStr := timeObj.Format("2006-01-02 15:04:05")
	fmt.Printf("%s\n", timeStr)

	// 字符串转Time对象
	timeStr = "2018-05-08 22:02:06"
	timeObj, _ = time.Parse("2006-01-02 15:04:05", timeStr)
	timeStamp = timeObj.Unix()
	fmt.Printf("%d\n", timeStamp)

	// Output:
	// 2018-05-08 22:02:06
	// 1525816926
}

////////////////////////////////////////////////////////////////////////////////
// 时区选择

func ExampleTimezone() {
	// 展示为上海时区
	loc, _ := time.LoadLocation("Asia/Shanghai")
	// 时间戳转Time对象
	var timeStamp int64 = 1525788126
	timeObj := time.Unix(timeStamp, 0)

	// Format() 前最好手动指定时区
	// fmt.Printf("time: %+v, string: %s\n", timeObj, timeObj.Format("2006-01-02 15:04:05"))
	fmt.Printf("time: %+v, string: %s\n", timeObj, timeObj.In(loc).Format("2006-01-02 15:04:05"))

	// Parse() 默认是使用 UTC 时区读入的
	timeStr := "2018-05-08 22:02:06"
	startAt, _ := time.Parse("2006-01-02 15:04:05", timeStr)
	fmt.Printf("time: %+v, cst time: %+v\n", startAt, startAt.In(loc))

	// 所以在读入时间字符串时, 应将 ParseInLocation() 作为首选项
	startAt2, _ := time.ParseInLocation("2006-01-02 15:04:05", timeStr, loc)
	fmt.Printf("time: %+v, cst time: %+v\n", startAt2, startAt2.In(loc))

	// Output:
	// time: 2018-05-08 07:02:06 -0700 PDT, string: 2018-05-08 22:02:06
	// time: 2018-05-08 22:02:06 +0000 UTC, cst time: 2018-05-09 06:02:06 +0800 CST
	// time: 2018-05-08 22:02:06 +0800 CST, cst time: 2018-05-08 22:02:06 +0800 CST
}
