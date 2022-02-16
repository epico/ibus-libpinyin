#!/usr/bin/awk

BEGIN {
    # Begin a transaction
    print "BEGIN TRANSACTION;"

    # Create english table
    print "CREATE TABLE IF NOT EXISTS \"strokes\" ( " \
        "\"character\" TEXT NOT NULL,"                 \
        "\"sequence\" INTEGER NOT NULL,"               \
        "\"strokes\" TEXT NOT NULL,"                   \
        "\"token\" INTEGER NOT NULL DEFAULT (0)"       \
        ");";

    # Create desc table
    print "CREATE TABLE IF NOT EXISTS desc (name TEXT PRIMARY KEY, value TEXT);";
    print "INSERT OR IGNORE INTO desc VALUES ('version', '1.2.0');";
}

    # Insert data into english table
NF == 4 {
        printf "INSERT INTO strokes (\"character\", \"sequence\", strokes, token) VALUES (\"%s\", %d, \"%s\", %d);\n", $1, $2, $3, $4;
        }

    #quit sqlite3
END {
    # Commit the transcation
    print "COMMIT;"
}
