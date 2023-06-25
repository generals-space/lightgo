package elf

// 	@compatible: 此函数在 v1.3 版本初次添加
//
// DynamicSymbols returns the dynamic symbol table for f.
// The symbols will be listed in the order they appear in f.
//
// For compatibility with Symbols, DynamicSymbols omits the null symbol at index 0.
// After retrieving the symbols as symtab, an externally supplied index x
// corresponds to symtab[x-1], not symtab[x].
func (f *File) DynamicSymbols() ([]Symbol, error) {
	sym, _, err := f.getSymbols(SHT_DYNSYM)
	return sym, err
}
