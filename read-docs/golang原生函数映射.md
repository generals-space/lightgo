```c++
// golang原生: println() 函数
void
runtime·printstring(String v)
{
	if(v.len > runtime·maxstring) {
		gwrite("[string too long]", 17);
		return;
	}
	if(v.len > 0)
		gwrite(v.str, v.len);
}
```

使用 vscode 全局搜索时, 需要将"包含的文件"显式设置为"*", 否则找不全.
