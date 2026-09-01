package main

/*
#include <stdlib.h>
*/
import "C"

import (
	"context"
	"fmt"
	"runtime"
	"sync"
	"unsafe"

	box "github.com/sagernet/sing-box"
	CBox "github.com/sagernet/sing-box/constant"
	"github.com/sagernet/sing-box/experimental/libbox"
	"github.com/sagernet/sing-box/include"
	"github.com/sagernet/sing-box/option"
	"github.com/sagernet/sing/common/json"
)

const abiVersion = 2

const (
	coreStatusStopped C.int = 0
	coreStatusRunning C.int = 1
)

var errorState struct {
	sync.RWMutex
	message string
}

var coreState struct {
	sync.Mutex
	instance *box.Box
	cancel   context.CancelFunc
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

func createInstance(configContent string) (*box.Box, context.CancelFunc, error) {
	ctx := box.Context(
		context.Background(),
		include.InboundRegistry(),
		include.OutboundRegistry(),
		include.EndpointRegistry(),
		include.DNSTransportRegistry(),
		include.ServiceRegistry(),
		include.CertificateProviderRegistry(),
	)
	options, err := json.UnmarshalExtendedContext[option.Options](ctx, []byte(configContent))
	if err != nil {
		return nil, nil, fmt.Errorf("decode config: %w", err)
	}
	ctx, cancel := context.WithCancel(ctx)
	instance, err := box.New(box.Options{Context: ctx, Options: options})
	if err != nil {
		cancel()
		return nil, nil, fmt.Errorf("create service: %w", err)
	}
	return instance, cancel, nil
}

func stopLocked() error {
	if coreState.instance == nil {
		return nil
	}
	instance := coreState.instance
	cancel := coreState.cancel
	coreState.instance = nil
	coreState.cancel = nil
	if cancel != nil {
		cancel()
	}
	if err := instance.Close(); err != nil {
		return fmt.Errorf("close service: %w", err)
	}
	return nil
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

//export leme_core_start
func leme_core_start(config *C.char) (result C.int) {
	defer recoverAsError(&result)
	if config == nil {
		return setLastError(fmt.Errorf("config must not be null"))
	}
	coreState.Lock()
	defer coreState.Unlock()
	if coreState.instance != nil {
		return setLastError(fmt.Errorf("core is already running"))
	}
	instance, cancel, err := createInstance(C.GoString(config))
	if err != nil {
		return setLastError(err)
	}
	if err = instance.Start(); err != nil {
		cancel()
		_ = instance.Close()
		return setLastError(fmt.Errorf("start service: %w", err))
	}
	coreState.instance = instance
	coreState.cancel = cancel
	return setLastError(nil)
}

//export leme_core_stop
func leme_core_stop() (result C.int) {
	defer recoverAsError(&result)
	coreState.Lock()
	defer coreState.Unlock()
	return setLastError(stopLocked())
}

//export leme_core_reload
func leme_core_reload(config *C.char) (result C.int) {
	defer recoverAsError(&result)
	if config == nil {
		return setLastError(fmt.Errorf("config must not be null"))
	}
	configContent := C.GoString(config)
	if err := libbox.CheckConfig(configContent); err != nil {
		return setLastError(fmt.Errorf("check reload config: %w", err))
	}
	coreState.Lock()
	defer coreState.Unlock()
	if err := stopLocked(); err != nil {
		return setLastError(err)
	}
	instance, cancel, err := createInstance(configContent)
	if err != nil {
		return setLastError(err)
	}
	if err = instance.Start(); err != nil {
		cancel()
		_ = instance.Close()
		return setLastError(fmt.Errorf("start reloaded service: %w", err))
	}
	coreState.instance = instance
	coreState.cancel = cancel
	return setLastError(nil)
}

//export leme_core_status
func leme_core_status() C.int {
	coreState.Lock()
	defer coreState.Unlock()
	if coreState.instance != nil {
		return coreStatusRunning
	}
	return coreStatusStopped
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
