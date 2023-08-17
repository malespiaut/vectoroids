const std = @import("std");
const Build = std.build;

pub fn build(b: *Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "vectoroids",
        .optimize = optimize,
        .target = target,
    });
    exe.addCSourceFiles(&.{
        "vectoroids.c",
    }, &.{
        "-std=c2x",
    });

    exe.linkSystemLibrary("c");
    exe.linkSystemLibrary("sdl2");
    exe.linkSystemLibrary("SDL2_image");
    exe.linkSystemLibrary("SDL2_mixer");

    exe.defineCMacro("DATA_PREFIX", "\"data/\"");
    exe.defineCMacro("JOY_NO", null);
    exe.defineCMacro("LINUX", null);

    b.installArtifact(exe);

    const play = b.step("play", "Play the game");
    const run = b.addRunArtifact(exe);
    run.step.dependOn(b.getInstallStep());
    play.dependOn(&run.step);
}
