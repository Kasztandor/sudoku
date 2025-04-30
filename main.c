#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

// Struktura przechowująca stan gry
typedef struct {
    int size;           // Rozmiar planszy (4, 9, 16)
    int box_size;       // Rozmiar podkwadratu (2, 3, 4)
    int** solution;     // Pełne rozwiązanie Sudoku
    int** puzzle;       // Plansza z usuniętymi wartościami (do rozwiązania)
    int** player_board; // Plansza gracza
    bool** fixed;       // Czy pole jest stałe (podpowiedź)
    int difficulty;     // Poziom trudności (1-łatwy, 2-średni, 3-trudny)
    int hints;          // Liczba podpowiedzi
    time_t start_time;  // Czas rozpoczęcia gry
} SudokuGame;

// Deklaracje funkcji
SudokuGame* create_game(int size, int difficulty);
void free_game(SudokuGame* game);
void generate_solution(SudokuGame* game);
void fill_diagonal_box(SudokuGame* game, int row, int col);
void remove_numbers(SudokuGame* game);
bool is_valid(SudokuGame* game, int row, int col, int num);
bool solve_sudoku(SudokuGame* game);
bool find_empty_cell(SudokuGame* game, int* row, int* col);
void print_board(SudokuGame* game);
void save_game(SudokuGame* game, const char* filename);
SudokuGame* load_game(const char* filename);
void play_game(SudokuGame* game);
void display_menu();
void show_instructions();
int count_solutions(SudokuGame* game, int count);
bool find_empty_cell_puzzle(SudokuGame* game, int* row, int* col);
bool is_valid_puzzle(SudokuGame* game, int row, int col, int num);
void give_hint(SudokuGame* game);
void display_stats(SudokuGame* game);
bool is_board_complete(SudokuGame* game);
bool is_valid_player(SudokuGame* game, int row, int col, int num);

int main() {
    srand(time(NULL)); // Inicjalizacja generatora liczb losowych
    
    int choice;
    SudokuGame* game = NULL;
    
    do {
        display_menu();
        printf("Wybierz opcję: ");
        scanf("%d", &choice);
        
        switch(choice) {
            case 1: { // Nowa gra
                int size, difficulty;
                
                printf("\nWybierz rozmiar planszy:\n");
                printf("1. 4x4 (łatwe)\n");
                printf("2. 9x9 (standardowe)\n");
                printf("3. 16x16 (trudne)\n");
                printf("Wybierz: ");
                scanf("%d", &size);
                
                printf("\nWybierz poziom trudności:\n");
                printf("1. Łatwy (więcej podpowiedzi)\n");
                printf("2. Średni\n");
                printf("3. Trudny (mniej podpowiedzi)\n");
                printf("Wybierz: ");
                scanf("%d", &difficulty);
                
                // Mapowanie wyboru użytkownika na rzeczywiste rozmiary
                if (size == 1) size = 4;
                else if (size == 2) size = 9;
                else if (size == 3) size = 16;
                else {
                    printf("Nieprawidłowy wybór, ustawiam 9x9\n");
                    size = 9;
                }
                
                if (difficulty < 1 || difficulty > 3) {
                    printf("Nieprawidłowy wybór, ustawiam średni\n");
                    difficulty = 2;
                }
                
                if (game != NULL) free_game(game);
                game = create_game(size, difficulty);
                generate_solution(game);
                remove_numbers(game);
                play_game(game);
                break;
            }
            case 2: { // Wczytaj grę
                char filename[100];
                printf("Podaj nazwę pliku do wczytania: ");
                scanf("%s", filename);
                
                if (game != NULL) free_game(game);
                game = load_game(filename);
                
                if (game != NULL) {
                    printf("Gra wczytana pomyślnie!\n");
                    play_game(game);
                } else {
                    printf("Nie udało się wczytać gry.\n");
                }
                break;
            }
            case 3: // Instrukcja
                show_instructions();
                break;
            case 4: // Wyjście
                if (game != NULL) free_game(game);
                printf("Dziękujemy za grę!\n");
                break;
            default:
                printf("Nieprawidłowy wybór, spróbuj ponownie.\n");
        }
    } while (choice != 4);
    
    return 0;
}

// Tworzenie nowej gry
SudokuGame* create_game(int size, int difficulty) {
    SudokuGame* game = (SudokuGame*)malloc(sizeof(SudokuGame));
    game->size = size;
    game->box_size = (int)sqrt(size);
    game->difficulty = difficulty;
    game->hints = 3; // Początkowa liczba podpowiedzi
    game->start_time = time(NULL);
    
    // Alokacja pamięci dla tablic
    game->solution = (int**)malloc(size * sizeof(int*));
    game->puzzle = (int**)malloc(size * sizeof(int*));
    game->player_board = (int**)malloc(size * sizeof(int*));
    game->fixed = (bool**)malloc(size * sizeof(bool*));
    
    for (int i = 0; i < size; i++) {
        game->solution[i] = (int*)malloc(size * sizeof(int));
        game->puzzle[i] = (int*)malloc(size * sizeof(int));
        game->player_board[i] = (int*)malloc(size * sizeof(int));
        game->fixed[i] = (bool*)malloc(size * sizeof(bool));
        
        for (int j = 0; j < size; j++) {
            game->solution[i][j] = 0;
            game->puzzle[i][j] = 0;
            game->player_board[i][j] = 0;
            game->fixed[i][j] = false;
        }
    }
    
    return game;
}

// Zwolnienie pamięci
void free_game(SudokuGame* game) {
    if (game == NULL) return;
    
    for (int i = 0; i < game->size; i++) {
        free(game->solution[i]);
        free(game->puzzle[i]);
        free(game->player_board[i]);
        free(game->fixed[i]);
    }
    
    free(game->solution);
    free(game->puzzle);
    free(game->player_board);
    free(game->fixed);
    free(game);
}

// Generowanie pełnego rozwiązania Sudoku
void generate_solution(SudokuGame* game) {
    // Wypełnij przekątną podkwadratów
    for (int box = 0; box < game->size; box += game->box_size) {
        fill_diagonal_box(game, box, box);
    }
    
    // Rozwiąż resztę planszy
    solve_sudoku(game);
    
    // Skopiuj rozwiązanie do planszy gracza
    for (int i = 0; i < game->size; i++) {
        for (int j = 0; j < game->size; j++) {
            game->player_board[i][j] = game->solution[i][j];
        }
    }
}

// Wypełnianie przekątnych podkwadratów
void fill_diagonal_box(SudokuGame* game, int row, int col) {
    int nums[game->size];
    for (int i = 0; i < game->size; i++) {
        nums[i] = i + 1;
    }
    
    // Tasowanie liczb
    for (int i = game->size - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = nums[i];
        nums[i] = nums[j];
        nums[j] = temp;
    }
    
    // Wypełnianie podkwadratu
    int index = 0;
    for (int i = 0; i < game->box_size; i++) {
        for (int j = 0; j < game->box_size; j++) {
            game->solution[row + i][col + j] = nums[index++];
            game->fixed[row + i][col + j] = true;
        }
    }
}

// Rozwiązywanie Sudoku (backtracking)
bool solve_sudoku(SudokuGame* game) {
    int row, col;
    
    if (!find_empty_cell(game, &row, &col)) {
        return true; // Brak pustych komórek - rozwiązane
    }
    
    // Próbuj liczby od 1 do size
    for (int num = 1; num <= game->size; num++) {
        if (is_valid(game, row, col, num)) {
            game->solution[row][col] = num;
            
            if (solve_sudoku(game)) {
                return true;
            }
            
            game->solution[row][col] = 0; // Backtrack
        }
    }
    
    return false;
}

// Znajdź pustą komórkę
bool find_empty_cell(SudokuGame* game, int* row, int* col) {
    for (*row = 0; *row < game->size; (*row)++) {
        for (*col = 0; *col < game->size; (*col)++) {
            if (game->solution[*row][*col] == 0) {
                return true;
            }
        }
    }
    return false;
}

// Sprawdzenie poprawności liczby w komórce
bool is_valid(SudokuGame* game, int row, int col, int num) {
    // Sprawdź wiersz
    for (int x = 0; x < game->size; x++) {
        if (game->solution[row][x] == num) {
            return false;
        }
    }
    
    // Sprawdź kolumnę
    for (int x = 0; x < game->size; x++) {
        if (game->solution[x][col] == num) {
            return false;
        }
    }
    
    // Sprawdź podkwadrat
    int box_start_row = row - row % game->box_size;
    int box_start_col = col - col % game->box_size;
    
    for (int i = 0; i < game->box_size; i++) {
        for (int j = 0; j < game->box_size; j++) {
            if (game->solution[box_start_row + i][box_start_col + j] == num) {
                return false;
            }
        }
    }
    
    return true;
}

// Usuwanie liczb z planszy, aby stworzyć zagadkę
void remove_numbers(SudokuGame* game) {
    int cells_to_remove;
    
    // Określ liczbę komórek do usunięcia na podstawie poziomu trudności
    switch (game->difficulty) {
        case 1: // Łatwy
            cells_to_remove = (int)(0.5 * game->size * game->size);
            break;
        case 2: // Średni
            cells_to_remove = (int)(0.6 * game->size * game->size);
            break;
        case 3: // Trudny
            cells_to_remove = (int)(0.7 * game->size * game->size);
            break;
        default:
            cells_to_remove = (int)(0.5 * game->size * game->size);
    }
    
    // Skopiuj rozwiązanie do zagadki
    for (int i = 0; i < game->size; i++) {
        for (int j = 0; j < game->size; j++) {
            game->puzzle[i][j] = game->solution[i][j];
            game->fixed[i][j] = false;
        }
    }
    
    // Usuwaj komórki, zapewniając jednoznaczność rozwiązania
    int count = 0;
    while (count < cells_to_remove) {
        int row = rand() % game->size;
        int col = rand() % game->size;
        
        if (game->puzzle[row][col] != 0) {
            int backup = game->puzzle[row][col];
            game->puzzle[row][col] = 0;
            
            // Sprawdź, czy plansza ma tylko jedno rozwiązanie
            int solutions = count_solutions(game, 0);
            
            if (solutions == 1) {
                game->fixed[row][col] = false;
                count++;
            } else {
                game->puzzle[row][col] = backup; // Przywróć wartość
            }
        }
    }
    
    // Skopiuj zagadkę do planszy gracza
    for (int i = 0; i < game->size; i++) {
        for (int j = 0; j < game->size; j++) {
            game->player_board[i][j] = game->puzzle[i][j];
            if (game->puzzle[i][j] != 0) {
                game->fixed[i][j] = true;
            }
        }
    }
}

// Liczenie liczby rozwiązań (ograniczone do 2 dla wydajności)
int count_solutions(SudokuGame* game, int count) {
    if (count > 1) return count; // Nie potrzebujemy dokładnej liczby
    
    int row, col;
    
    if (!find_empty_cell_puzzle(game, &row, &col)) {
        return count + 1;
    }
    
    for (int num = 1; num <= game->size && count < 2; num++) {
        if (is_valid_puzzle(game, row, col, num)) {
            game->puzzle[row][col] = num;
            count = count_solutions(game, count);
            game->puzzle[row][col] = 0;
        }
    }
    
    return count;
}

// Wersje funkcji dla planszy zagadki
bool find_empty_cell_puzzle(SudokuGame* game, int* row, int* col) {
    for (*row = 0; *row < game->size; (*row)++) {
        for (*col = 0; *col < game->size; (*col)++) {
            if (game->puzzle[*row][*col] == 0) {
                return true;
            }
        }
    }
    return false;
}

bool is_valid_puzzle(SudokuGame* game, int row, int col, int num) {
    // Sprawdź wiersz
    for (int x = 0; x < game->size; x++) {
        if (game->puzzle[row][x] == num) {
            return false;
        }
    }
    
    // Sprawdź kolumnę
    for (int x = 0; x < game->size; x++) {
        if (game->puzzle[x][col] == num) {
            return false;
        }
    }
    
    // Sprawdź podkwadrat
    int box_start_row = row - row % game->box_size;
    int box_start_col = col - col % game->box_size;
    
    for (int i = 0; i < game->box_size; i++) {
        for (int j = 0; j < game->box_size; j++) {
            if (game->puzzle[box_start_row + i][box_start_col + j] == num) {
                return false;
            }
        }
    }
    
    return true;
}

// Wyświetlanie planszy
void print_board(SudokuGame* game) {
    printf("\n");
    for (int i = 0; i < game->size; i++) {
        if (i % game->box_size == 0 && i != 0) {
            for (int j = 0; j < game->size + (game->size / game->box_size) - 1; j++) {
                printf("- ");
            }
            printf("\n");
        }
        
        for (int j = 0; j < game->size; j++) {
            if (j % game->box_size == 0 && j != 0) {
                printf("| ");
            }
            
            if (game->player_board[i][j] == 0) {
                printf(". ");
            } else {
                if (game->fixed[i][j]) {
                    printf("\033[1m%d \033[0m", game->player_board[i][j]); // Pogrubione dla podpowiedzi
                } else {
                    printf("%d ", game->player_board[i][j]);
                }
            }
        }
        printf("\n");
    }
    printf("\n");
}

// Główna pętla gry
void play_game(SudokuGame* game) {
    int row, col, num;
    char command[10];
    bool game_over = false;
    
    while (!game_over) {
        system("clear"); // Czyść ekran (dla Linux/Mac)
        // system("cls"); // Dla Windows
        
        print_board(game);
        display_stats(game);
        
        printf("Dostępne komendy:\n");
        printf("set <wiersz> <kolumna> <wartość> - ustaw wartość\n");
        printf("del <wiersz> <kolumna> - usuń wartość\n");
        printf("hint - podpowiedź (pozostało: %d)\n", game->hints);
        printf("save <nazwa_pliku> - zapisz grę\n");
        printf("exit - wyjście do menu\n");
        printf("Komenda: ");
        
        scanf("%s", command);
        
        if (strcmp(command, "set") == 0) {
            scanf("%d %d %d", &row, &col, &num);
            
            if (row < 1 || row > game->size || col < 1 || col > game->size || 
                num < 1 || num > game->size) {
                printf("Nieprawidłowe dane wejściowe!\n");
                continue;
            }
            
            if (game->fixed[row-1][col-1]) {
                printf("Nie możesz zmienić podpowiedzi!\n");
                continue;
            }
            
            game->player_board[row-1][col-1] = num;
            
            // Sprawdź, czy gracz wygrał
            if (is_board_complete(game)) {
                printf("\nGratulacje! Rozwiązałeś Sudoku!\n");
                print_board(game);
                game_over = true;
            }
        } 
        else if (strcmp(command, "del") == 0) {
            scanf("%d %d", &row, &col);
            
            if (row < 1 || row > game->size || col < 1 || col > game->size) {
                printf("Nieprawidłowe dane wejściowe!\n");
                continue;
            }
            
            if (game->fixed[row-1][col-1]) {
                printf("Nie możesz usunąć podpowiedzi!\n");
                continue;
            }
            
            game->player_board[row-1][col-1] = 0;
        } 
        else if (strcmp(command, "hint") == 0) {
            if (game->hints > 0) {
                give_hint(game);
                game->hints--;
            } else {
                printf("Brak dostępnych podpowiedzi!\n");
            }
        } 
        else if (strcmp(command, "save") == 0) {
            char filename[100];
            scanf("%s", filename);
            save_game(game, filename);
            printf("Gra zapisana w pliku %s\n", filename);
        } 
        else if (strcmp(command, "exit") == 0) {
            game_over = true;
        } 
        else {
            printf("Nieznana komenda!\n");
        }
        
        // Czekaj na reakcję użytkownika
        if (!game_over) {
            printf("Naciśnij Enter, aby kontynuować...");
            while (getchar() != '\n'); // Czyść bufor wejścia
            getchar();
        }
    }
}

// Sprawdzenie, czy plansza jest kompletna i poprawna
bool is_board_complete(SudokuGame* game) {
    // Sprawdź, czy wszystkie komórki są wypełnione
    for (int i = 0; i < game->size; i++) {
        for (int j = 0; j < game->size; j++) {
            if (game->player_board[i][j] == 0) {
                return false;
            }
        }
    }
    
    // Sprawdź poprawność wierszy, kolumn i podkwadratów
    for (int i = 0; i < game->size; i++) {
        for (int j = 0; j < game->size; j++) {
            int num = game->player_board[i][j];
            game->player_board[i][j] = 0;
            
            if (!is_valid_player(game, i, j, num)) {
                game->player_board[i][j] = num;
                return false;
            }
            
            game->player_board[i][j] = num;
        }
    }
    
    return true;
}

// Wersja funkcji is_valid dla planszy gracza
bool is_valid_player(SudokuGame* game, int row, int col, int num) {
    // Sprawdź wiersz
    for (int x = 0; x < game->size; x++) {
        if (game->player_board[row][x] == num) {
            return false;
        }
    }
    
    // Sprawdź kolumnę
    for (int x = 0; x < game->size; x++) {
        if (game->player_board[x][col] == num) {
            return false;
        }
    }
    
    // Sprawdź podkwadrat
    int box_start_row = row - row % game->box_size;
    int box_start_col = col - col % game->box_size;
    
    for (int i = 0; i < game->box_size; i++) {
        for (int j = 0; j < game->box_size; j++) {
            if (game->player_board[box_start_row + i][box_start_col + j] == num) {
                return false;
            }
        }
    }
    
    return true;
}

// Podpowiedź - pokazuje poprawną wartość w losowej komórce
void give_hint(SudokuGame* game) {
    int empty_cells = 0;
    
    // Policz puste komórki
    for (int i = 0; i < game->size; i++) {
        for (int j = 0; j < game->size; j++) {
            if (game->player_board[i][j] == 0) {
                empty_cells++;
            }
        }
    }
    
    if (empty_cells == 0) {
        printf("Brak pustych komórek!\n");
        return;
    }
    
    // Wybierz losową pustą komórkę
    int hint_index = rand() % empty_cells;
    int current_index = 0;
    
    for (int i = 0; i < game->size; i++) {
        for (int j = 0; j < game->size; j++) {
            if (game->player_board[i][j] == 0) {
                if (current_index == hint_index) {
                    game->player_board[i][j] = game->solution[i][j];
                    printf("Podpowiedź: wiersz %d, kolumna %d powinna mieć wartość %d\n", 
                           i+1, j+1, game->solution[i][j]);
                    return;
                }
                current_index++;
            }
        }
    }
}

// Zapisywanie gry do pliku
void save_game(SudokuGame* game, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("Nie można otworzyć pliku do zapisu.\n");
        return;
    }
    
    // Zapisz podstawowe informacje o grze
    fprintf(file, "%d %d %d %ld\n", game->size, game->difficulty, game->hints, time(NULL) - game->start_time);
    
    // Zapisz planszę rozwiązania
    for (int i = 0; i < game->size; i++) {
        for (int j = 0; j < game->size; j++) {
            fprintf(file, "%d ", game->solution[i][j]);
        }
        fprintf(file, "\n");
    }
    
    // Zapisz planszę zagadki
    for (int i = 0; i < game->size; i++) {
        for (int j = 0; j < game->size; j++) {
            fprintf(file, "%d ", game->puzzle[i][j]);
        }
        fprintf(file, "\n");
    }
    
    // Zapisz planszę gracza
    for (int i = 0; i < game->size; i++) {
        for (int j = 0; j < game->size; j++) {
            fprintf(file, "%d ", game->player_board[i][j]);
        }
        fprintf(file, "\n");
    }
    
    // Zapisz informacje o stałych komórkach
    for (int i = 0; i < game->size; i++) {
        for (int j = 0; j < game->size; j++) {
            fprintf(file, "%d ", game->fixed[i][j] ? 1 : 0);
        }
        fprintf(file, "\n");
    }
    
    fclose(file);
}

// Wczytywanie gry z pliku
SudokuGame* load_game(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Nie można otworzyć pliku do odczytu.\n");
        return NULL;
    }
    
    int size, difficulty, hints;
    long time_elapsed;
    
    if (fscanf(file, "%d %d %d %ld\n", &size, &difficulty, &hints, &time_elapsed) != 4) {
        printf("Błąd odczytu nagłówka pliku.\n");
        fclose(file);
        return NULL;
    }
    
    SudokuGame* game = create_game(size, difficulty);
    game->hints = hints;
    game->start_time = time(NULL) - time_elapsed;
    
    // Wczytaj planszę rozwiązania
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if (fscanf(file, "%d ", &game->solution[i][j]) != 1) {
                printf("Błąd odczytu planszy rozwiązania.\n");
                fclose(file);
                free_game(game);
                return NULL;
            }
        }
    }
    
    // Wczytaj planszę zagadki
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if (fscanf(file, "%d ", &game->puzzle[i][j]) != 1) {
                printf("Błąd odczytu planszy zagadki.\n");
                fclose(file);
                free_game(game);
                return NULL;
            }
        }
    }
    
    // Wczytaj planszę gracza
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if (fscanf(file, "%d ", &game->player_board[i][j]) != 1) {
                printf("Błąd odczytu planszy gracza.\n");
                fclose(file);
                free_game(game);
                return NULL;
            }
        }
    }
    
    // Wczytaj informacje o stałych komórkach
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            int fixed;
            if (fscanf(file, "%d ", &fixed) != 1) {
                printf("Błąd odczytu stałych komórek.\n");
                fclose(file);
                free_game(game);
                return NULL;
            }
            game->fixed[i][j] = (fixed == 1);
        }
    }
    
    fclose(file);
    return game;
}

// Wyświetlanie menu głównego
void display_menu() {
    printf("\n=== MENU GŁÓWNE SUDOKU ===\n");
    printf("1. Nowa gra\n");
    printf("2. Wczytaj grę\n");
    printf("3. Instrukcja\n");
    printf("4. Wyjście\n");
}

// Wyświetlanie instrukcji
void show_instructions() {
    printf("\n=== INSTRUKCJA GRY W SUDOKU ===\n");
    printf("Cel gry: Wypełnij planszę cyframi tak, aby:\n");
    printf("- W każdym wierszu każda cyfra występowała tylko raz\n");
    printf("- W każdej kolumnie każda cyfra występowała tylko raz\n");
    printf("- W każdym podkwadracie (oznaczonym grubszą linią) każda cyfra występowała tylko raz\n\n");
    printf("Podczas gry możesz używać następujących komend:\n");
    printf("set <wiersz> <kolumna> <wartość> - ustaw wartość w podanej komórce\n");
    printf("del <wiersz> <kolumna> - usuń wartość z podanej komórki\n");
    printf("hint - poproś o podpowiedź (liczba podpowiedzi jest ograniczona)\n");
    printf("save <nazwa_pliku> - zapisz aktualny stan gry do pliku\n");
    printf("exit - wyjdź do menu głównego\n\n");
    printf("Naciśnij Enter, aby wrócić do menu...");
    while (getchar() != '\n'); // Czyść bufor wejścia
    getchar();
}

// Wyświetlanie statystyk gry
void display_stats(SudokuGame* game) {
    time_t current_time = time(NULL);
    int seconds = (int)difftime(current_time, game->start_time);
    int minutes = seconds / 60;
    seconds %= 60;
    
    printf("Czas gry: %02d:%02d | Podpowiedzi: %d | Rozmiar: %dx%d\n", 
           minutes, seconds, game->hints, game->size, game->size);
    printf("Poziom trudności: ");
    switch (game->difficulty) {
        case 1: printf("Łatwy"); break;
        case 2: printf("Średni"); break;
        case 3: printf("Trudny"); break;
        default: printf("Nieznany");
    }
    printf("\n\n");
}