consense command just opens consense(Scrapbox) page 

run following to install (stable)
$ brew tap kota-yata/consense https://github.com/kota-yata/consense
$ brew install consense

consense set-project [project-name]
consense [page-name]                  // opens the page with your default browser
consense [page-name] "[content]"       // opens the page and pre-fills content
consense compress [backup JSON file path] [distinatino JSON file path] // compress backup file
consense decompress [compressed JSON file path] // decompress compressed file

Notes for content argument:
- Must be a single, double-quoted string.
- Supports backslash escapes: \n, \t, \\, \", \r, \b, \f, \0
- Example: consense "meeting notes" "Title\\n- item 1\\n- item 2"
