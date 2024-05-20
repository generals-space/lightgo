package http_test

import (
	"bytes"
	"encoding/json"
	"io/ioutil"
	"log"
	"net/http"
)

// import "io/ioutil"

func ExampleGet() {
	addr := "https://www.baidu.com"
	// 直接发送Get请求, res为Response类型
	res, err := http.Get(addr)
	if err != nil {
		panic(err)
	}
	// 这一句貌似是必须的
	defer res.Body.Close()
	/*
	 * header是Header类型, 而Header本身的定义为
	 * type Header map[string][]string
	 * 所以可以用很常规的方法获取到响应头信息
	 */
	header := res.Header
	for k, v := range header {
		log.Printf("key: %s, value: %s", k, v)
	}
	log.Println("============================================")
	// Body是一个类似于Buffer的类型, 有两个方法可以读取其中的内容.
	// 1. 如果使用res.Body自己的Read方法, 就必须创建一个容器来承接
	// 但实际上不一定读取1024个字节, 有可能存在多于或少于的情况.
	bodyCnt := make([]byte, 1024)
	length, err := res.Body.Read(bodyCnt)
	log.Printf("length: %d\n", length)
	log.Printf("%s\n", bodyCnt)

	// 2. 也可以用ioutil库来读取内容
	// body, err := ioutil.ReadAll(res.Body)
	// log.Printf("%s\n", body)
}

type User struct {
	Name string
	Age  int
}

func ExamplePost() {
	addr := "http://localhost:8080"

	// Post() 适合发送二进制数据, 如文件, 图片, json数据等.
	// PostForm() 发送的是表单格式`application/x-www-form-urlencoded`的数据.

	// jsonData := url.Values{
	// 	"name": {"general"},
	// 	"age":  {"21"},
	// }
	// res, err := http.PostForm(addr, jsonData)

	user := User{
		Name: "general",
		Age:  21,
	}
	data, err := json.Marshal(user)
	if err != nil {
		panic(err)
	}
	jsonData := bytes.NewBuffer(data)
	res, err := http.Post(addr, "application/json", jsonData)
	if err != nil {
		panic(err)
	}
	// 这一句貌似是必须的
	defer res.Body.Close()

	body, err := ioutil.ReadAll(res.Body)
	log.Printf("%s\n", body)
}

func ExampleGetRequest() {
	client := &http.Client{}
    url := "https://www.baidu.com"
    req, err := http.NewRequest("GET", url, nil)
    if err != nil {
        panic(err)
    }
	// req.Header.Set("User-Agent", "curl/7.54.0")

    res, err := client.Do(req)

    result, err := ioutil.ReadAll(res.Body)
    log.Printf("%s\n", result)
}

////////////////////////////////////////////////////////////////////////////////
// LoginInfo ...
type LoginInfo struct {
	Action string `json:"action"`
	Params struct {
		LoginID string `json:"login_id"`
		Passwd  string `json:"passwd"`
		Type    int    `json:"type"`
	} `json:"params"`
}
// LoginRespPayload ...
type LoginRespPayload struct{
	Token string	`json:"token"`
}
// LoginResp ...
type LoginResp struct {
	Token  string `json:"token"`
	Code   int    `json:"code"`
	Msg    string `json:"msg"`
	Total  int `json:"total"`
	Result LoginRespPayload `json:"result"`
}

func ExamplePostRequest() {
	serverAddr := "http://sapigs.vipvoice.link/user/checkin.boxuds"
	client := &http.Client{}

	// loginInfo := make(map[string]interface{})
	// loginInfo["action"] = "user/checkin"
	// loginInfo["params"] = map[string]interface{}{
	// 	"login_id": "13073197649",
	// 	"passwd": "123456",
	// 	"type": 0,
	// }
	loginInfo := &LoginInfo{
		Action: "user/checkin",
	}
	loginInfo.Params.LoginID = "13073197649"
	loginInfo.Params.Passwd = "123456"
	loginInfo.Params.Type = 0

	bytesData, err := json.Marshal(loginInfo)
	if err != nil {
		log.Fatal(err)
		return
	}
	// log.Println(string(bytesData))
	// return
	jsonData := bytes.NewReader(bytesData)
	req, err := http.NewRequest("POST", serverAddr, jsonData)

	if err != nil {
		log.Fatal(err)
	}
	req.Header.Set("Content-Type", "application/json")
	req.Header.Set("Referer", "http://ip.vipvoice.link/login?redirect=%2F")
	req.Header.Set("User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_2) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/68.0.3440.106 Safari/537.36")

	rsp, err := client.Do(req)
	if err != nil {
		log.Fatal(err)
	}
	defer rsp.Body.Close()

	rspData, err := ioutil.ReadAll(rsp.Body)
	if err != nil {
		log.Fatal(err)
	}
	log.Println(string(rspData))
	loginRsp := &LoginResp{}
	json.Unmarshal(rspData, loginRsp)
	log.Printf("%+v", loginRsp)
}
