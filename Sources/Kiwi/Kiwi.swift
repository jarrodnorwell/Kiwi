//
//  Kiwi.swift
//
//
//  Created by Jarrod Norwell on 21/2/2024.
//

import KiwiObjC
import Foundation

public struct Kiwi {
    public static let shared = Kiwi()
    
    fileprivate let kiwiObjC = KiwiObjC.shared()
    
    public func insert(rom url: URL) {
        kiwiObjC.insert(game: url)
    }
    
    public func step() {
        kiwiObjC.step()
    }
    
    public func screenFramebuffer() -> UnsafeMutablePointer<UInt32> {
        kiwiObjC.screenFramebuffer()
    }
    
    public func virtualControllerButtonDown(_ button: UInt8) {
        kiwiObjC.virtualControllerButtonDown(button)
    }
    
    public func virtualControllerButtonUp(_ button: UInt8) {
        kiwiObjC.virtualControllerButtonUp(button)
    }
}
