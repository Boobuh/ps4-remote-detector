#!/usr/bin/env swift
import Foundation

let host = ProcessInfo.processInfo.environment["PS4_HOST"] ?? "10.0.1.7"
let paths = [
    "/data/ps4-remote-detector/boot.log",
    "/data/ps4-remote-detector/events.log",
    "/data/ps4-remote-detector/map.csv",
]

for path in paths {
    print("\n========== \(path) ==========")
    let p = Process()
    p.executableURL = URL(fileURLWithPath: "/usr/bin/curl")
    p.arguments = ["-sf", "ftp://\(host):2121\(path)"]
    let pipe = Pipe()
    p.standardOutput = pipe
    p.standardError = Pipe()
    try? p.run()
    p.waitUntilExit()
    let data = pipe.fileHandleForReading.readDataToEndOfFile()
    if data.isEmpty {
        print("(missing)")
    } else if let s = String(data: data, encoding: .utf8) {
        print(s)
    }
}
