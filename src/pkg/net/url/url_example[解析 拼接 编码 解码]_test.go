package url_test

import (
	"fmt"
	"net/url"
	"strings"
)

////////////////////////////////////////////////////////////////////////////////
// URL解析
//
// net/url 包中关于解析的 Parse 函数有3个, 看名字就可以明白其作用:
//
// 1. Parse()
// 2. ParseQuery()
// 3. ParseRequestURI()

func ExampleParse() {
	// 这个URL包含了一个 scheme, 认证信息, 主机名, 端口, 路径, 查询参数和片段.
	var urlStr string = "https://user:pass@www.baidu.com:8080/path?key1=val1&key2=val2#frag"
	urlObj, _ := url.Parse(urlStr)

	fmt.Printf("scheme: %s\n", urlObj.Scheme) // https

	// User属性包含了所有的认证信息, 这里调用Username()和Password()方法来获取独立值.
	user := urlObj.User
	username := user.Username()
	password, _ := user.Password()
	fmt.Printf("userinfo: %s\n", user)                             // user:pass
	fmt.Printf("username: %s, password: %s\n", username, password) // user, pass

	fmt.Println()

	// Host 同时包括主机名和端口信息, 如端口存在的话, 使用 strings.Split() 从 Host 中手动提取端口.
	fmt.Printf("fully address: %s\n", urlObj.Host) // www.baidu.com:8080
	hostArray := strings.Split(urlObj.Host, ":")
	fmt.Printf("host: %s, port: %s\n", hostArray[0], hostArray[1]) // www.baidu.com, 8080
	fmt.Printf(
		"hostname: %s, port: %s, path: %s, fragment: %s\n",
		urlObj.Hostname(), urlObj.Port(), urlObj.Path, urlObj.Fragment,
	) // www.baidu.com, 8080(字符串类型), /path, frag

	fmt.Println()

	// RawQuery()可以得到a=1&b=2原始查询字符串, 不包含问号
	fmt.Printf("raw query: %s\n", urlObj.RawQuery) // key1=val1&key2=val2
	// Query()方法得到查询参数 map, 格式见示例.
	// queryMap为 url.Values 对象, 而ta其实是一个 map[string][]string 别名.
	queryMap := urlObj.Query()
	fmt.Printf("key1: %s, key2: %s\n", queryMap["key1"][0], queryMap["key2"][0]) // key1: val1, key2: val2

	// url.ParseQuery() 可以解析指定的查询参数字符串, 常用, 但是 queryString 不能带问号, 否则会报错
	var queryString string = "key1=val1&key2=val2"
	queryMap2, _ := url.ParseQuery(queryString)
	fmt.Printf("key1: %s, key2: %s\n", queryMap2["key1"][0], queryMap2["key2"][0]) // key1: val1, key2: val2

	// Output:
	// scheme: https
	// userinfo: user:pass
	// username: user, password: pass
	//
	// fully address: www.baidu.com:8080
	// host: www.baidu.com, port: 8080
	// hostname: www.baidu.com, port: 8080, path: /path, fragment: frag
	//
	// raw query: key1=val1&key2=val2
	// key1: val1, key2: val2
	// key1: val1, key2: val2
}

////////////////////////////////////////////////////////////////////////////////
// URL拼接
//

// 拼接可用 URL 对象的ResolveReference()方法

func ExampleMerge() {
	urlStr := "https://www.baidu.com/path1/path2?key1=val1&key2=val2#frag"
	urlObj, _ := url.Parse(urlStr)

	subURL := "path3?key1=subquery#subfrag"
	subURLObj, _ := url.Parse(subURL)
	fullURLObj := urlObj.ResolveReference(subURLObj)
	fmt.Println(fullURLObj.String())

	// Output:
	// https://www.baidu.com/path1/path3?key1=subquery#subfrag
}

////////////////////////////////////////////////////////////////////////////////
// 编解码
//
// 1. urlObj.EscapedPath()
// 2. url.QueryEscape(str)
// 3. url.PathEscape(str)
