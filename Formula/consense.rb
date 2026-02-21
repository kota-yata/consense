class Consense < Formula
  desc "Open Scrapbox pages from the CLI"
  homepage "https://github.com/kota-yata/consense"
  head "https://github.com/kota-yata/consense.git", branch: "main"

  # For a stable release (recommended over head-only for public taps):
  # 1) Tag a release in your repo (e.g., v0.1.0)
  # 2) Uncomment url/sha256 below with the release tarball and checksum
  # url "https://github.com/kota-yata/consense/archive/refs/tags/v0.1.0.tar.gz"
  # sha256 "REPLACE_WITH_SHA256"

  def install
    system "make"
    bin.install "consense"
  end

  test do
    system bin/"consense", "set-project", "brewtest"
    assert_match "brewtest", (testpath/".consense_project").read
  end
end
