package user

// 	@compatible: 此函数在 v1.7 版本添加
//
// Group represents a grouping of users.
//
// On POSIX systems Gid contains a decimal number
// representing the group ID.
type Group struct {
	Gid  string // group ID
	Name string // group name
}

// UnknownGroupIdError is returned by LookupGroupId when
// a group cannot be found.
type UnknownGroupIdError string

func (e UnknownGroupIdError) Error() string {
	return "group: unknown groupid " + string(e)
}

// 	@compatible: 此函数在 v1.7 版本添加
//
// LookupGroupId looks up a group by groupid. If the group cannot be found, the
// returned error is of type UnknownGroupIdError.
func LookupGroupId(gid string) (*Group, error) {
	return lookupGroupId(gid)
}
