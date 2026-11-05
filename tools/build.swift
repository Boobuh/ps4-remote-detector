#!/usr/bin/env swift
import Foundation

let repo = URL(fileURLWithPath: #filePath).deletingLastPathComponent().deletingLastPathComponent()
let appDir = repo.appendingPathComponent("app", isDirectory: true)
let ps4Pro = repo.deletingLastPathComponent().appendingPathComponent("ps4-linux-pro", isDirectory: true)

@discardableResult
func run(_ cmd: String, _ args: [String], cwd: URL) throws -> Int32 {
    let p = Process()
    p.executableURL = URL(fileURLWithPath: cmd)
    p.arguments = args
    p.currentDirectoryURL = cwd
    var env = ProcessInfo.processInfo.environment
    env["OO_PS4_TOOLCHAIN"] = ps4Pro.appendingPathComponent("vendor/OpenOrbis/PS4Toolchain").path
    env["PATH"] = "/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin"
    p.environment = env
    p.standardOutput = FileHandle.standardOutput
    p.standardError = FileHandle.standardError
    try p.run()
    p.waitUntilExit()
    return p.terminationStatus
}

let make = "/usr/bin/make"
let status = try run(make, [], cwd: appDir)
if status != 0 {
    fputs("build failed\n", stderr)
    exit(1)
}

let pkg = try FileManager.default.contentsOfDirectory(at: appDir, includingPropertiesForKeys: nil)
    .first { $0.pathExtension == "pkg" }
if let pkg {
    print("Built: \(pkg.path)")
}
