#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ID_SIZE 4
#define USERNAME_SIZE 32
#define EMAIL_SIZE 255

#define ID_OFFSET 0
#define USERNAME_OFFSET 4
#define EMAIL_OFFSET 36
#define ROW_SIZE 291

#define PAGE_SIZE 4096
#define TABLE_MAX_PAGE 100
#define ROWS_PER_PAGE 14
#define TABLE_MAX_ROWS 1400

typedef struct InputBuffer_t {
    size_t bufferLength;
    ssize_t inputLength;
    char *buffer;
} InputBuffer;

typedef struct Row_t {
    uint32_t id;
    char username[USERNAME_SIZE + 1];
    char email[EMAIL_SIZE + 1];
} Row;

typedef struct Table_t {
    uint32_t numRows;
    uint32_t x; // Not used just to silence compiler warning - padding releited
    void *pages[TABLE_MAX_PAGE]; // TABLE_MAX_PAGE - can't use const here
} Table;


enum MetaCommandResult_t {
    META_COMMAND_SUCCESS,
    META_COMMAND_RECOGNISED_COMMAND
};
typedef enum MetaCommandResult_t MetaCommandResult;

enum PrepareResult_t {
    PREPARE_SUCCESS,
    PREPADE_NEGATIVE_ID,
    PREPARE_RECOGNISED_STATMENT,
    PREPARE_SYNTAX_ERROR,
    PREPARE_STRING_TOO_LONG
};
typedef enum PrepareResult_t PrepareResult;

enum StatmentType_t {
    STATMENT_INSERT,
    STATMENT_SELECT
};
typedef enum StatmentType_t StatmentType;

typedef struct Statment_t {
    StatmentType type;
    Row rowToInsert;
} Statment;

enum ExecuteResult_t {
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL
};
typedef enum ExecuteResult_t ExecuteResult;
PrepareResult prepareInsert(InputBuffer *inputBuffer, Statment *statment);
Table *newTable(void);
void printRow(Row *row);
ExecuteResult executeInsert(Statment *statment, Table *table);
ExecuteResult executeSelect(Table *table);
void *rowSlot(Table *table, uint32_t rowNums);
void serializeRow(Row *source, void *destination);
void deSerializeRow(void *source, Row *destination);
MetaCommandResult doMetaCommand(InputBuffer *inputBuffer);
PrepareResult prepareStatment(InputBuffer *inputBuffer, Statment *statment);
ExecuteResult executeStatment(Statment *statment, Table *table);
void printPromt(void);
// Initiolases the input buffer
InputBuffer *newInputBuffer(void);
void readInput(InputBuffer *inputBuffer);

int main(void)
{
    Table *table = newTable();
    InputBuffer *inputBuffer = newInputBuffer();

    while(1) {
        printPromt();
        readInput(inputBuffer);
        if (inputBuffer->buffer[0] == '.') {
            switch (doMetaCommand(inputBuffer)) {
            case META_COMMAND_SUCCESS:
                continue;
            case META_COMMAND_RECOGNISED_COMMAND:
                printf("Unrecognised command %s\n", inputBuffer->buffer);
                continue;
            }
        }
        Statment statment;
        switch (prepareStatment(inputBuffer, &statment)) {
        case PREPARE_SUCCESS:
            break;
        case PREPARE_RECOGNISED_STATMENT:
            printf("Unrecognized statment %s\n", inputBuffer->buffer);
            continue;
        case PREPARE_SYNTAX_ERROR:
            printf("Syntax error\n");
            continue;
        case PREPARE_STRING_TOO_LONG:
            printf("String is too long.\n");
            continue;

        }
        switch (executeStatment(&statment, table)) {
        case EXECUTE_SUCCESS:
            printf("Executed!\n");
            break;
        case EXECUTE_TABLE_FULL:
            printf("Table is fill!\n");
            break;
        }
    }
    return 0;
}
void printPromt()
{
    printf("sqlClone >>");
}

InputBuffer *newInputBuffer(void)
{
    InputBuffer *ptr = (InputBuffer*)malloc(sizeof (InputBuffer));
    ptr->buffer = NULL;
    ptr->inputLength = 0;
    ptr->bufferLength = 0;
    return ptr;
}

void readInput(InputBuffer *inputBuffer)
{
    ssize_t tmpBuffer = getline(&(inputBuffer->buffer),
                                &(inputBuffer->bufferLength), stdin);
    if (tmpBuffer <= 0) {
        printf("Error reading input\n");
        exit(EXIT_FAILURE);
    }
    inputBuffer->inputLength = tmpBuffer - 1;
    inputBuffer->buffer[tmpBuffer - 1] = 0;
}

MetaCommandResult doMetaCommand(InputBuffer *inputBuffer)
{
    if (strcmp(inputBuffer->buffer, ".exit") == 0) {
        exit(EXIT_SUCCESS);
    } else {
        return META_COMMAND_RECOGNISED_COMMAND;
    }
}

PrepareResult prepareStatment(InputBuffer *inputBuffer, Statment *statment)
{
    if (strncmp(inputBuffer->buffer, "insert", 6) == 0) {
        return prepareInsert(inputBuffer, statment);
    } else if (strcmp(inputBuffer->buffer, "select") == 0) {
        statment->type = STATMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    return PREPARE_RECOGNISED_STATMENT;
}

ExecuteResult executeStatment(Statment *statment, Table *table)
{
    switch (statment->type) {
    case STATMENT_INSERT:
        return executeInsert(statment, table);
    case STATMENT_SELECT:
        return executeSelect(table);
    }
}
void serializeRow(Row *source, void *destination)
{
    memcpy((char*)destination + ID_OFFSET, &(source->id), ID_SIZE);
    memcpy((char*)destination + USERNAME_OFFSET, &(source->username),
                                                        USERNAME_SIZE);
    memcpy((char*)destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}
void deSerializeRow(void *source, Row *destination)
{
    memcpy(&(destination->id), (char*)source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), (char*)source + USERNAME_OFFSET,
                                                        USERNAME_SIZE);
    memcpy(&(destination->email), (char*)source + EMAIL_OFFSET, EMAIL_SIZE);
}
void *rowSlot(Table *table, uint32_t rowNums)
{
    uint32_t pageNum = rowNums / ROWS_PER_PAGE;
    void *page = table->pages[pageNum];
    if (!page) {
        page = table->pages[pageNum] = malloc(PAGE_SIZE);
    }
    uint32_t rowOffset = rowNums % ROWS_PER_PAGE;
    uint32_t byteOffset = rowOffset * ROW_SIZE;
    return (char*)page + byteOffset;
}
ExecuteResult executeInsert(Statment *statment, Table *table)
{
    if (table->numRows == TABLE_MAX_ROWS) {
        return EXECUTE_TABLE_FULL;
    }
    Row *rowToInsert = &(statment->rowToInsert);
    serializeRow(rowToInsert, rowSlot(table, table->numRows));
    table->numRows++;
    return EXECUTE_SUCCESS;
}
ExecuteResult executeSelect(Table *table)
{
    Row row;
    for (uint32_t i = 0; i < table->numRows; ++i) {
        deSerializeRow(rowSlot(table, i), &row);
        printRow(&row);
    }
    return EXECUTE_SUCCESS;
}
void printRow(Row *row)
{
    printf("%d, %s, %s\n", row->id, row->username, row->email);
}
Table *newTable()
{
    Table *table;
    table = (Table*) malloc(sizeof (Table));
    table->numRows = 0;
    return table;
}

PrepareResult prepareInsert(InputBuffer *inputBuffer, Statment *statment)
{
    statment->type = STATMENT_INSERT;

    char *keyword = strtok(inputBuffer->buffer, " ");
    char *idString = strtok(NULL, " ");
    char *username = strtok(NULL, " ");
    char *email = strtok(NULL, " ");

    if (idString == NULL || username == NULL || email == NULL) {
        return PREPARE_SYNTAX_ERROR;
    }

    int32_t id = atoi(idString);

    if (strlen(username) > USERNAME_SIZE) {
        return PREPARE_STRING_TOO_LONG;
    }
    if (strlen(email) > EMAIL_SIZE) {
        return PREPARE_STRING_TOO_LONG;
    }
    statment->rowToInsert.id = (uint32_t)id;
    strcpy(statment->rowToInsert.username, username);
    strcpy(statment->rowToInsert.email, email);
    return PREPARE_SUCCESS;
}
