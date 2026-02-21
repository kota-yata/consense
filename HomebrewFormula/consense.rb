class Consense < Formula
  desc "Open Scrapbox pages from the CLI"
  homepage "https://github.com/USER/consense"
  head "https://github.com/USER/consense.git", branch: "main"

  # For a stable release, create a GitHub release and fill these:
  # url "https://github.com/USER/consense/archive/refs/tags/v0.1.0.tar.gz"
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

