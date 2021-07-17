# This is done to pin the packages to a "stable" version
# Of nixpkgs.
# { pkgs ? import (fetchTarball "https://github.com/NixOS/nixpkgs/archive/nixos-21.05.tar.gz") {} }:

#{ piBuild ? false }:

#let
#  piSystem = {
#    config = "aarch64-unknown-linux-gnu";
#  };
#in

with import <nixpkgs> {};
#{ stdenv, lib, cmake }:

stdenv.mkDerivation {
  pname = "phoned";
  version = "1.0.0";

  # Only copy src when running nix-build.
  src = if lib.inNixShell then null else builtins.path {
    path = ./.; name = "phoned";
  };

  enableParallelBuilding = true;

  buildInputs = [
    cmake
  ];
}
