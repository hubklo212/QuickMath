#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

// Struktura reprezentująca gracza
typedef struct {
    char username[16];
    int score;
    double totalTime;
} Player;

// Funkcja obsługująca błędy SQLite
void handleSQLiteError(int rc, sqlite3* db) {
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQLite error: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(EXIT_FAILURE);
    }
}

// Inicjalizacja bazy danych
void initDatabase(sqlite3* db) {
    char* createTableSQL = "CREATE TABLE IF NOT EXISTS players ("
                           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                           "username TEXT NOT NULL,"
                           "score INTEGER NOT NULL,"
                           "totalTime REAL NOT NULL);";

    int rc = sqlite3_exec(db, createTableSQL, 0, 0, 0);
    handleSQLiteError(rc, db);
}

// Dodanie nowego gracza do bazy danych
void addPlayer(sqlite3* db, Player player) {
    char insertSQL[150];
    snprintf(insertSQL, sizeof(insertSQL), "INSERT INTO players (username, score, totalTime) VALUES ('%s', %d, %f);",
             player.username, player.score, player.totalTime);

    int rc = sqlite3_exec(db, insertSQL, 0, 0, 0);
    handleSQLiteError(rc, db);

    printf("Score recorded successfully.\n");
}

// Wyświetlenie rankingu graczy
void displayRanking(sqlite3* db) {
	char selectSQL[] = "SELECT * FROM players ORDER BY score DESC, totalTime ASC;";
	int index = 1;
	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare_v2(db, selectSQL, -1, &stmt, 0);
	handleSQLiteError(rc, db);

	printf("Ranking graczy:\n");
	printf("| %-3s | %-15s | %-6s | %-10s |\n", "", "Username", "Score", "Total Time");
	printf("|---|-----------------|--------|------------|\n");
	
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
	printf("| %-3d | %-15s | %-6d | %-10f |\n",
		index,
		sqlite3_column_text(stmt, 1),
		sqlite3_column_int(stmt, 2),
		sqlite3_column_double(stmt, 3));
	index++;
	}

	sqlite3_finalize(stmt);
}

// Przesłanie rankingu do gracza
void sendRankingToClient(sqlite3* db, int sockfd) {

	char selectSQL[] = "SELECT * FROM players ORDER BY score DESC, totalTime ASC;";

	sqlite3_stmt* stmt;

	int rc = sqlite3_prepare_v2(db, selectSQL, -1, &stmt, 0);

	if (rc != SQLITE_OK) {
	handleSQLiteError(rc, db);
	return;
	}

	char buffer[512];
	int index = 1;

	sprintf(buffer, "Leaderboard:\n| %-3s | %-15s | %-6s | %-10s |\n|-----|-----------------|--------|------------|\n",
            "", "Username", "Score", "Total Time");
    	send(sockfd, buffer, strlen(buffer), 0);

    	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        	sprintf(buffer, "| %-3d | %-15s | %-6d | %-10f |\n",
                	index,
                	sqlite3_column_text(stmt, 1),
                	sqlite3_column_int(stmt, 2),
                	sqlite3_column_double(stmt, 3));
        	index++;
        	send(sockfd, buffer, strlen(buffer), 0);
    	}

    send(sockfd, "", 1, 0);

    sqlite3_finalize(stmt);
}

/* // examples of use
int main() {
    sqlite3* db;
    int rc = sqlite3_open("game_database.db", &db);  // Użyj ":memory:" dla bazy w pamięci, można też podać ścieżkę do pliku na dysku

    handleSQLiteError(rc, db);
    initDatabase(db);

    Player player1 = {"Gracz5", 100, 120};
    Player player2 = {"mlody g", 80, 150};
    Player player3 = {"Gracz3", 120, 90};

    addPlayer(db, player1);
    addPlayer(db, player2);
    addPlayer(db, player3);


    displayRanking(db);

    sqlite3_close(db);
    return 0;
}
*/
