class Consense < Formula
  desc "Open Scrapbox pages from the CLI"
  homepage "https://github.com/kota-yata/consense"
  head "https://github.com/kota-yata/consense.git", branch: "main"

  # Recommended: provide a stable release (preferred by Homebrew):
  # 1) Tag a release in https://github.com/kota-yata/consense (e.g., v0.1.0)
  # 2) Uncomment url/sha256 below and set the correct checksum
  # url "https://github.com/kota-yata/consense/archive/refs/tags/v0.1.0.tar.gz"
  # sha256 "REPLACE_WITH_SHA256"

  def install
    system "make"
    bin.install "consense"
  end

  test do
    # Homebrew sets HOME to testpath in brew test
    system bin/"consense", "set-project", "brewtest"
    assert_match "brewtest", (testpath/".consense_project").read
  end
end

