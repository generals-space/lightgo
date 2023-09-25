package json_test

import (
	"encoding/json"
	"fmt"
)

// ColorGroup 参考了 encode_test.json 的测试内容.
type ColorGroup struct {
	ID     int
	Name   string
	Colors []string
}

func ExampleMarshalIndent() {
	group := ColorGroup{
		ID:     1,
		Name:   "Reds",
		Colors: []string{"Crimson", "Red", "Ruby", "Maroon"},
	}
	// 将结构体对象格式化为 json 字符串, 设置缩进为4个空格(也可以用 tab 键 "\t")
	//
	// result, err := json.MarshalIndent(group, "", "    ")
	result, err := json.MarshalIndent(group, "", "    ")
	if err != nil {
		fmt.Println("error:", err)
	}
	fmt.Printf("%s\n", result)

	// Output:
	// {
	//     "ID": 1,
	//     "Name": "Reds",
	//     "Colors": [
	//         "Crimson",
	//         "Red",
	//         "Ruby",
	//         "Maroon"
	//     ]
	// }
}
