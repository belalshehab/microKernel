import os

proc nimListenerOnGossip(data: ptr uint8, dataSize: csize_t, signature: ptr uint8, signatureSize: csize_t):
    cint {.importc, cdecl.}


proc nimListenerInit(port: uint16) {.exportc, cdecl.} =
    # Initialize the listener on the specified port
    echo "[Nim] Listener initialized on port: ", port
    var fakeData: array[4, uint8] = [1'u8, 2'u8, 3'u8, 4'u8]
    var fakeSignature: array[4, uint8] = [5'u8, 6'u8, 7'u8, 8'u8]
    for i in 1..3:
        sleep(2000)
        echo "[Nim] Simulating incoming gossip message: #", i
        let result = nimListenerOnGossip(
            addr fakeData[0], fakeData.len.csize_t,
            addr fakeSignature[0], fakeSignature.len.csize_t
        )
        echo "[Nim] onGossip returned: ", result



proc nimListenerOnValidated(isValid: bool,  validatorSignature: ptr uint8, validatorSignatureSize: csize_t) {.exportc, cdecl.} =
    echo "[Nim] Validation result: ", isValid, " with validator signature size: ", validatorSignatureSize
