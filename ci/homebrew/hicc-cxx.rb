# typed: false
# frozen_string_literal: true

#
# brew install hedzr/brew/hicc-cxx
#

# Documentation: https://docs.brew.sh/Formula-Cookbook
#                https://rubydoc.brew.sh/Formula
class HiccCxx < Formula
  desc "'hicc' is a c++ template class library to provide some basic data structures and algorithms"
  homepage "https://github.com/hedzr/hicc"
  url "https://github.com/hedzr/hicc.git",
      tag:      "0.2.1",
      revision: "asdf"
  sha256 "asdf"
  version_scheme 1
  head "https://github.com/hedzr/hicc.git"
  license "MIT"

#   bottle do
#     sha256 cellar: :any_skip_relocation, catalina:    "asdf"
#     sha256 cellar: :any_skip_relocation, high_sierra: "asdf"
#     sha256 cellar: :any_skip_relocation, sierra:      "asdf"
#     sha256 cellar: :any_skip_relocation, el_capitan:  "asdf"
#   end

  depends_on "cmake" => :build
  depends_on "catch2" => [:build, :test]
  # depends_on "make" => :build
  # # depends_on "scons" => :run
  # # depends_on "pkg-config" => :run
  # # gcc@9
  # depends_on "clang-format" => :recommended
  # depends_on "gcc" => :recommended
  # # depends_on "mingw-w64" => :recommended
  # # depends_on "wine" => :optional
  # depends_on "gprof2dot" => :optional
  # depends_on "graphviz" => :optional
  # depends_on "valgrind" => :optional
  # depends_on "yaml-cpp" => :optional
  depends_on :xcode => "9.3"
  conflicts_with "cmdr-cxx", because: "cmdr-cxx also ships the most of codes of hicc-cxx."

  def install
    ENV.deparallelize # if your formula fails when building in parallel
    # rm "LICENSE"

    # system "make", "PREFIX=#{prefix}", "install"
    system "cmake", "-S", ".", "-B", "build", *std_cmake_args
    system "cmake", "--build", "build"
    system "cmake", "--install", "build"

    # make a binary installing directly:
    # # system "tar -Jvxf awesome-tool-darwin-0.2.3.tar.zx"
    # bin.install 'awesome-tool' # copy the executable into /usr/local/bin/
    bin.install 'bin/rel/hicc-cli'
  end

  test do
    # `test do` will create, run in and delete a temporary directory.
    #
    # This test will fail and we won't accept that! For Homebrew/homebrew-core
    # this will need to be a test that verifies the functionality of the
    # software. Run the test with `brew test hicc-cxx-new`. Options passed
    # to `brew install` such as `--HEAD` also need to be provided to `brew test`.
    #
    # The installed folder is not in the path, so use the entire path to any
    # executables being tested: `system "#{bin}/program", "do", "something"`.
    #
    # system "false"
    outputs = shell_output("#{bin}/hicc-cli")
    assert_match(/Hello, World!/m, outputs)
  end
end
