package main

/*
#include <stdlib.h>
*/
import "C"

import (
	"fmt"
	"runtime"
	"sync"
	"unsafe"

	CBox "github.com/sagernet/sing-box/constant"
	"github.com/sagernet/sing-box/experimental/libbox"
)

const abiVersion = 1

var errorState struct {
	sync.RWMutex
	message string
}

func setLastError(err error) C.int {
	errorState.Lock()
	defer errorState.Unlock()
	if err == nil {
		errorState.message = ""
		return 0
	}
	errorState.message = err.Error()
	return 1
}

func recoverAsError(result *C.int) {
	if value := recover(); value != nil {
		*result = setLastError(fmt.Errorf("panic recovered at ABI boundary: %v", value))
	}
}

//export leme_core_abi_version
func leme_core_abi_version() C.int {
	return C.int(abiVersion)
}

//export leme_core_singbox_version
func leme_core_singbox_version() *C.char {
	return C.CString(CBox.Version)
}

//export leme_core_go_version
func leme_core_go_version() *C.char {
	return C.CString(runtime.Version() + " " + runtime.GOOS + "/" + runtime.GOARCH)
}

//export leme_core_check_config
func leme_core_check_config(config *C.char) (result C.int) {
	defer recoverAsError(&result)
	if config == nil {
		return setLastError(fmt.Errorf("config must not be null"))
	}
	return setLastError(libbox.CheckConfig(C.GoString(config)))
}

//export leme_core_last_error
func leme_core_last_error() *C.char {
	errorState.RLock()
	defer errorState.RUnlock()
	return C.CString(errorState.message)
}

//export leme_core_free_string
func leme_core_free_string(value *C.char) {
	if value != nil {
		C.free(unsafe.Pointer(value))
	}
}

func main() {}
