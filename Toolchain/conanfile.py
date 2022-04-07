import os
from conans import ConanFile, tools

class L4TToolchainConan(ConanFile):
    name = "gcc_pi4"
    version = "10.3.0"
    settings = "os", "arch", "compiler", "build_type"

    def source(self):
        url = "https://sourceforge.net/projects/raspberry-pi-cross-compilers/files/Raspberry%20Pi%20GCC%20Cross-Compiler%20Toolchains/Bullseye/GCC%2010.3.0/Raspberry%20Pi%203A%2B%2C%203B%2B%2C%204/cross-gcc-10.3.0-pi_3%2B.tar.gz/download"
        self.output.info("Downloading %s" % url)
        tools.download(url, "toolchain.tar.gz")
    
    def build(self):
        toolchain_path = os.path.join(self.source_folder, "toolchain.tar.xz")
        self.output.info("Extracting Toolchain")
        self.run("tar xf %s -C %s --strip-components=1" % (toolchain_path, self.build_folder))
        os.remove(toolchain_path)

    def package(self):
        self.copy("*", dst="", src=self.build_folder)

    def package_info(self):
        bin_folder = os.path.join(self.package_folder, "bin")
        self.env_info.CC = os.path.join(bin_folder, "aarch64-linux-gnu-gcc")
        self.env_info.CXX = os.path.join(bin_folder, "aarch64-linux-gnu-g++")
        self.env_info.GDB = os.path.join(bin_folder, "aarch64-linux-gnu-gdb")
        self.env_info.LD = os.path.join(bin_folder, "aarch64-linux-gnu-ld")
        self.env_info.NM = os.path.join(bin_folder, "aarch64-linux-gnu-nm")
        self.env_info.AR = os.path.join(bin_folder, "aarch64-linux-gnu-ar")
        self.env_info.AS = os.path.join(bin_folder, "aarch64-linux-gnu-as")
        self.env_info.STRIP = os.path.join(bin_folder, "aarch64-linux-gnu-strip")
        self.env_info.RANLIB = os.path.join(bin_folder, "aarch64-linux-gnu-ranlib")
        self.env_info.STRINGS = os.path.join(bin_folder, "aarch64-linux-gnu-strings")
        self.env_info.OBJDUMP = os.path.join(bin_folder, "aarch64-linux-gnu-objdump")
        self.env_info.OBJCOPY = os.path.join(bin_folder, "aarch64-linux-gnu-objcopy")
        self.env_info.GCOV = os.path.join(bin_folder, "aarch64-linux-gnu-gcov")
        self.env_info.SYSROOT = os.path.join(self.package_folder, "aarch64-linux-gnu/libc")
        self.env_info.CONAN_CMAKE_SYSROOT = os.path.join(self.package_folder, "aarch64-linux-gnu/libc")
