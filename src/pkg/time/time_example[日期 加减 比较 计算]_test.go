package time_test

import (
	"fmt"
	"time"
)

////////////////////////////////////////////////////////////////////////////////
// 日期比较
//
// 比较两个日期谁先谁后

func ExampleCompare() {
	// 作为示例, 就不去获取当前时间了, 没有固定的输出, 不好做测试.
	// now := time.Now()
	now, _ := time.Parse("2006-01-02 15:04:05", "2020-01-01 00:00:00")
	fmt.Printf("now: %s\n", now)

	// 创建一个Date时间对象
	then, _ := time.Parse("2006-01-02 15:04:05", "2012-01-01 00:00:00")
	fmt.Printf("then: %s\n", then)

	// Before, After, Equal比较两个时间对象, 在秒级单位判断谁先谁后
	fmt.Printf("then < now ? %t\n", then.Before(now))
	fmt.Printf("then > now ? %t\n", then.After(now))
	fmt.Printf("then = now ? %t\n", then.Equal(now))

	// Output:
	//
	// now: 2020-01-01 00:00:00 +0000 UTC
	// then: 2012-01-01 00:00:00 +0000 UTC
	// then < now ? true
	// then > now ? false
	// then = now ? false
}

////////////////////////////////////////////////////////////////////////////////
// 日期加减
//
// 如下示例展示了两个时间点的比较方法, 以及通过`Time`对象与`Duration`对象的加减操作得到
// 新的`Time`对象或`Duration`对象.
//
// 1. `Time`对象 - `Time`对象, 得到`Duration`对象.
// 2. `Time`对象 +|- `Duration`对象, 得到新的`Time`对象.

func ExampleAddAndSub() {
	// 作为示例, 就不去获取当前时间了, 没有固定的输出, 不好做测试.
	// now := time.Now()
	now, _ := time.Parse("2006-01-02 15:04:05", "2020-01-01 00:00:00")
	fmt.Printf("now: %s\n", now)

	// 创建一个Date时间对象
	then, _ := time.Parse("2006-01-02 15:04:05", "2012-01-01 00:00:00")
	fmt.Printf("then: %s\n", then)

	// 两个Date对象相减得到Duration对象, 表示两个日期之间间隔的长度.
	delta := now.Sub(then)
	fmt.Printf("delta type: %T\n", delta)
	// 一个Date对象加上一个Duration对象得到一个新的Date对象
	future := now.Add(delta)
	fmt.Printf("future type: %T\n", future)
	fmt.Printf("future: %s\n", future)

	fmt.Println()

	// 也可以用一个负号-表示减去Duration
	then2 := now.Add(-delta)
	fmt.Printf("then2 type: %T\n", then2)
	fmt.Printf("then2: %s\n", then2)

	// 我们可以获取一个Duration对象一共有几个小时, 或几分钟, 几秒
	fmt.Printf(
		"delta is %f Hours, or %f Minutes, or %f Seconds \n",
		delta.Hours(), delta.Minutes(), delta.Seconds(),
	)

	// Output:
	//
	// now: 2020-01-01 00:00:00 +0000 UTC
	// then: 2012-01-01 00:00:00 +0000 UTC
	// delta type: time.Duration
	// future type: time.Time
	// future: 2028-01-01 00:00:00 +0000 UTC
	//
	// then2 type: time.Time
	// then2: 2012-01-01 00:00:00 +0000 UTC
	// delta is 70128.000000 Hours, or 4207680.000000 Minutes, or 252460800.000000 Seconds
}

////////////////////////////////////////////////////////////////////////////////
// 日期计算
//
// 有的时候, 我们知道一个时间点, 只想直接获取到它的前/后的一小时, 一天或一周的时间点.
//
// 这种情况下, 我们需要直接构建出`Duration`对象(再找两个`time`对象相减就不太好了吧).
//
// `time`包里构建`Duration`对象可以用`time.ParseDuration()`函数.

func ExampleParseDuration() {
	// 作为示例, 就不去获取当前时间了, 没有固定的输出, 不好做测试.
	// now := time.Now()
	now, _ := time.Parse("2006-01-02 15:04:05", "2020-01-01 00:00:00")
	fmt.Printf("now: %s\n", now)

	// 可用单位只有 "ns", "us" (or "µs"), "ms", "s", "m", "h"
	d1, _ := time.ParseDuration("24h")

	oneHourLater := now.Add(d1)
	fmt.Printf("一天后: %s\n", oneHourLater)

	oneYearLater := now.Add(d1 * 365)
	fmt.Printf("一年后: %s\n", oneYearLater)

	// 也可以直接使用`time.Second`, `time.Hour`, 不使用`ParseDuration()`.
	twoHourLater := now.Add(time.Hour * 2)
	fmt.Printf("两个小时后: %s\n", twoHourLater)

	// Output:
	//
	// now: 2020-01-01 00:00:00 +0000 UTC
	// 一天后: 2020-01-02 00:00:00 +0000 UTC
	// 一年后: 2020-12-31 00:00:00 +0000 UTC
	// 两个小时后: 2020-01-01 02:00:00 +0000 UTC
}
