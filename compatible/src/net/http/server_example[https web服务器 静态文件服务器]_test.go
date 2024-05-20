package http_test

import (
	"encoding/json"
	"io"
	"io/ioutil"
	"log"
	"net/http"
	"strings"
)

////////////////////////////////////////////////////////////////////////////////
// 一个简单的 http server

// hello world, the web server
func HelloServer(w http.ResponseWriter, req *http.Request) {
	io.WriteString(w, "hello, world!\n")
}

func ExampleListenAndServe() {
	http.HandleFunc("/hello", HelloServer)
	err := http.ListenAndServe(":12345", nil)
	if err != nil {
		log.Fatal("ListenAndServe: ", err)
	}
}

////////////////////////////////////////////////////////////////////////////////
// 一个简单的 https server

func handler(w http.ResponseWriter, req *http.Request) {
	w.Header().Set("Content-Type", "text/plain")
	w.Write([]byte("This is an example server.\n"))
}

func ExampleListenAndServeTLS() {
	http.HandleFunc("/", handler)
	log.Printf("About to listen on 10443. Go to https://127.0.0.1:10443/")
	err := http.ListenAndServeTLS(":10443", "cert.pem", "key.pem", nil)
	if err != nil {
		log.Fatal(err)
	}
}

////////////////////////////////////////////////////////////////////////////////

// getHandler 获取get请求参数
func getHandler(resp http.ResponseWriter, req *http.Request) {
	query := req.URL.Query()
	name := query["name"][0]
	title := query["title"]
	log.Println(name)
	log.Println(title)

	data := map[string]interface{}{
		"name":  name,
		"title": title,
	}
	result, _ := json.Marshal(data)
	resp.Write([]byte(result))
}

// postHandler 获取post请求json数据
func postHandler(resp http.ResponseWriter, req *http.Request) {
	defer req.Body.Close()
	byteData, _ := ioutil.ReadAll(req.Body)
	log.Printf("%s\n", byteData)
	// var mapData map[string]interface{}
	mapData := map[string]interface{}{}
	err := json.Unmarshal(byteData, &mapData)
	if err != nil {
		log.Println(err)
	}

	mapResult := map[string]interface{}{
		"username": mapData["username"],
		"password": mapData["password"],
	}
	result, _ := json.Marshal(mapResult)
	resp.Write(result)
}

func ExampleHttpServer() {
	http.HandleFunc("/get", getHandler)
	http.HandleFunc("/post", postHandler)
	log.Println("http server started...")
	http.ListenAndServe(":8080", nil)
}

/**
$ curl 'localhost:8079/get?name=general&title=ceo&title=manager'
{"name":"general","title":["ceo","manager"]}
$ curl -X POST -d '{"username": "general", "password": "123456"}' localhost:8079/post
{"password":"123456","username":"general"}
**/

////////////////////////////////////////////////////////////////////////////////
// 静态文件服务器

func ExampleFileServer() {
	log.Fatal(http.ListenAndServe(":8080", http.FileServer(http.Dir("/usr/share/doc"))))
}

// 访问 localhost:8080/tmpfiles/ 可以看到`/tmp`目录下的文件列表
func ExampleStripPrefix() {
	// StripPrefix() 的使用是必要的, 否则访问 localhost:8080/tmpfiles/ 会得到404.
	//
	// 如果不使用 StripPrefix(),
	// http.Handle("/tmpfiles/", http.FileServer(http.Dir("/tmp")))
	// "/tmpfiles/" 相当于nginx中的`location`值, 而`http.Dir("/tmp")`类似于nginx中的`root`字段值.
	// 当用户访问`/tmpfiles/`时, 其实是访问`/tmp/tmpfiles/`目录.
	// (可以在 /tmp 目录下手动创建 tmpfiles 子目录, 试试看)
	//
	// 注意路由路径字符串中尾部的斜线`/`, 如果是目录最好加上.
	//
	// To serve a directory on disk (/tmp) under an alternate URL path (/tmpfiles/),
	// use StripPrefix to modify the request URL's path before the FileServer sees it:
	http.Handle("/tmpfiles/", http.StripPrefix("/tmpfiles/", http.FileServer(http.Dir("/tmp"))))
	http.ListenAndServe(":8080", nil)
}

// 自定义安全策略, 如仅限登录用户访问, 或者限制`referer`头(反爬, 防盗链等)
func fileServerHandler(resp http.ResponseWriter, req *http.Request) {
	log.Printf("访问静态文件: %s\n", req.URL.Path)
	if strings.HasSuffix(req.URL.Path, ".ini") {
		resp.Write([]byte("禁止访问!"))
		return
	}
	http.FileServer(http.Dir("/tmp")).ServeHTTP(resp, req)
}

func ExampleFileServer2() {
	http.HandleFunc("/tmpfiles/", fileServerHandler)
	log.Println("http server started...")
	http.ListenAndServe(":8080", nil)
}

////////////////////////////////////////////////////////////////////////////////
