package time_test

import (
	"fmt"
	"time"
)

/*

`ticker`类似于js中的`setInterval`, 一般设置时间间隔, 然后循环调用;

而`timer`则类似于`setTimeout`, 一般设置延时时间, 只能单次触发.

*/

////////////////////////////////////////////////////////////////////////////////
// Ticker

func ExampleTicker1() {
	fmt.Println("start")
	var counter int
	ticker := time.NewTicker(time.Second * 1)
	for {
		<-ticker.C
		// 每隔1秒打印一次
		fmt.Println("work")
		counter += 1
		if counter >= 3 {
			ticker.Stop()
			break
		}
	}
	fmt.Println("end")

	// Output:
	// start
	// work
	// work
	// work
	// end
}

func ExampleTicker2() {
	fmt.Println("start")
	var counter int
	// 另一种创建 ticker 对象的方法
	tickerChannel := time.Tick(time.Second * 1)
	for {
		// now 为触发 ticker 的当前时间
		// now := <-tickerChannel
		_ = <-tickerChannel
		fmt.Println("work")
		counter += 1
		// fmt.Println(now)
		if counter >= 3 {
			break
		}
	}
	fmt.Println("end")

	// Output:
	// start
	// work
	// work
	// work
	// end
}

////////////////////////////////////////////////////////////////////////////////
// Timer

func ExampleTimer1() {
	// 第1种 Timer
	fmt.Println("start")
	timer := time.NewTimer(time.Second * 1)
	<-timer.C
	fmt.Println("1s后")
	// 此时如果再次 <-timer.C会阻塞, 需要reset重新设置定时器
	fmt.Println("end")

	fmt.Println()

	// 第2种 Timer
	// After() 返回一个channel, 在指定时间d后可读, 类似于timer对象中的C成员.
	fmt.Println("start")
	tChannel := time.After(time.Second * 1)
	// now 为触发 timer 的当前时间
	// now := <-tChannel
	_ = <-tChannel
	fmt.Println("1s后")
	fmt.Println("end")

	fmt.Println()

	// 第3种 Timer
	// AfterFunc() 要执行的函数是在独立的协程中触发的, 因为主协程中需要多等待一会
	fmt.Println("start")
	time.AfterFunc(time.Second*1, func() {
		fmt.Println("1s后")
	})
	time.Sleep(time.Second * 2)
	fmt.Println("2s后")
	fmt.Println("end")

	// Output:
	// start
	// 1s后
	// end
	//
	// start
	// 1s后
	// end
	//
	// start
	// 1s后
	// 2s后
	// end
}

func ExampleTimer2() {
	fmt.Println("start")
	timer := time.NewTimer(time.Second * 1)
	<-timer.C
	fmt.Println("1s后")

	var counter int
	for {
		// 此时如果再次 <-timer.C会阻塞, 需要reset重新设置定时器
		//
		// 与`setTimeout()`相似, 如果要实现类似`setInterval()`循环执行函数的行为,
		// 需要在循环中重新设置`setTimeout()`.
		//
		// 而`timer`可以通过`Reset()`方法重置`timer`对象, 然后下一次到达指定时间又可以执行.
		//
		timer.Reset(time.Second * 1)
		<-timer.C
		fmt.Println("work")
		counter += 1
		if counter >= 3 {
			timer.Stop()
			break
		}
	}
	fmt.Println("end")
	// Output:
	// start
	// 1s后
	// work
	// work
	// work
	// end
}
