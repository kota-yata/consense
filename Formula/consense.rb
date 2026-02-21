class Consense < Formula
  desc "Open Scrapbox pages from the CLI"
  homepage "https://github.com/kota-yata/consense"
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
