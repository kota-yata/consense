Tap for Homebrew: consense

Repo name (recommended): homebrew-consense

How to create and publish:

1) Create an empty GitHub repo: https://github.com/kota-yata/homebrew-consense

2) Locally, publish the tap:

   cd tap-homebrew-consense
   git init
   git add .
   git commit -m "consense formula"
   git branch -M main
   git remote add origin git@github.com:kota-yata/homebrew-consense.git
   git push -u origin main

3) Users install via:

   brew tap kota-yata/consense
   brew install consense

Alternative (install from raw formula URL without tapping):

   brew install https://raw.githubusercontent.com/kota-yata/homebrew-consense/main/Formula/consense.rb

Stable release (recommended):

1) In https://github.com/kota-yata/consense, create a tag, e.g. v0.1.0
   git tag v0.1.0 && git push origin v0.1.0

2) Update Formula/consense.rb in this tap repo:
   - Uncomment url and sha256
   - Compute checksum locally:
     curl -L -o v0.1.0.tar.gz https://github.com/kota-yata/consense/archive/refs/tags/v0.1.0.tar.gz
     shasum -a 256 v0.1.0.tar.gz
   - Paste the sha256 value into the formula

