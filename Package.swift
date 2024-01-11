// swift-tools-version:5.9
import PackageDescription

let package = Package(
    name: "mth",
    products: [
        .library(
            name: "mth",
            targets: ["mth"]),
        .library(
            name: "mthcolor",
            targets: ["mthcolor"]),
    ],
    targets: [
        .target(
            name: "mth",
            dependencies: [],
            path: "Sources/mth",
            sources: ["msdp.c", "mth.c", "telopt.c", "net.c", "mud.c"],
            publicHeadersPath: "./"
        ),
        .target(
            name: "mthcolor",
            dependencies: [],
            path: "Sources/color",
            sources: ["color.c"],
            publicHeadersPath: "./"
        )
    ]
)

