package mime

var (
	// 	@compatible: 此变量在 v1.5 版本添加
	//
	// extensions maps from MIME type to list of lowercase file
	// extensions: "image/jpeg" => [".jpg", ".jpeg"]
	extensions map[string][]string
)

// 	@compatible: 此函数在 v1.5 版本添加
//
// ExtensionsByType returns the extensions known to be associated with the MIME
// type typ. The returned extensions will each begin with a leading dot, as in
// ".html". When typ has no associated extensions, ExtensionsByType returns an
// nil slice.
func ExtensionsByType(typ string) ([]string, error) {
	justType, _, err := ParseMediaType(typ)
	if err != nil {
		return nil, err
	}

	once.Do(initMime)
	mimeLock.RLock()
	defer mimeLock.RUnlock()
	s, ok := extensions[justType]
	if !ok {
		return nil, nil
	}
	return append([]string{}, s...), nil
}
