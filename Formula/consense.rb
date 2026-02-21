class Consense < Formula
  desc "Open Scrapbox pages from the CLI"
  homepage "https://github.com/kota-yata/consense"
  url "https://github.com/kota-yata/consense/archive/refs/tags/v0.1.2.tar.gz"
  sha256 "a76591acc44d74a3fc065ad6c87bf5a9b96310192743ac45a16f388849cffe1b"
  head "https://github.com/kota-yata/consense.git", branch: "main"

  def install
    system "make"
    bin.install "consense"
  end

  test do
    system bin/"consense", "set-project", "brewtest"
    assert_match "brewtest", (testpath/".consense_project").read
    # Running without a browser should fail; assert URL appears in stderr
    out = shell_output("#{bin}/consense p \"A\\nB\" 2>&1", 1)
    assert_match %r{https://scrapbox.io/brewtest/p\?body=A%0AB}, out
  end
end
