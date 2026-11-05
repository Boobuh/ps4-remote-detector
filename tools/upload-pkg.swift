#!/usr/bin/env swift
import Foundation

let repo = URL(fileURLWithPath: #filePath).deletingLastPathComponent().deletingLastPathComponent()
let appDir = repo.appendingPathComponent("app", isDirectory: true)
let host = ProcessInfo.processInfo.environment["PS4_HOST"] ?? "10.0.1.7"

guard let pkg = try FileManager.default.contentsOfDirectory(at: appDir, includingPropertiesForKeys: nil)
    .first(where: { $0.pathExtension == "pkg" }) else {
    fputs("No .pkg in app/ — run swift tools/build.swift first\n", stderr)
    exit(1)
}

let remote = "ftp://\(host):2121/data/pkg/\(pkg.lastPathComponent)"
let p = Process()
p.executableURL = URL(fileURLWithPath: "/usr/bin/curl")
p.arguments = ["-T", pkg.path, "--ftp-create-dirs", remote]
p.standardOutput = FileHandle.standardOutput
p.standardError = FileHandle.standardError
try p.run()
p.waitUntilExit()
if p.terminationStatus == 0 {
    print("Uploaded → \(remote)")
    print("Package Installer → /data/pkg/\(pkg.lastPathComponent)")
}
exit(p.terminationStatus)
