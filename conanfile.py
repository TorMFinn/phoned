from conans import ConanFile, CMake, tools


class PhonedConan(ConanFile):
    name = "phoned"
    version = "0.1.0"
    license = "Proprietary"
    author = "Tor Morten Finnesand tmf@kvstech.no"
    url = "https://github.com/tormfinn/phoned"
    description = "Old Phones"
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = ["cmake_paths", "cmake_find_package"]

    requires = [
            "boost/1.77.0",
            "sdl/2.0.18",
    ]

    exports_sources = [
            "CMakeLists.txt",
            "cmake/*",
            "etc/*",
            "ext/*",
            "include/*",
            "src/*"
    ]

    def configure(self):
        self.options["sdl"].nas = False
        self.options["sdl"].pulse = False
        self.options["sdl"].wayland = False
        self.options["sdl"].opengl = False
        self.options["sdl"].opengles = False
        self.options["sdl"].vulkan = False
        self.options["sdl"].libunwind = False
        self.options["sdl"].iconv = False

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["libphoned"]

