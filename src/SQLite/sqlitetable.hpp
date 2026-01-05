#pragma once

#include "databasecommon.hpp"

#include <list>
#include <map>

namespace Database {

class SQLiteDatabase;
class SQLiteExecutor;

/**
 * @brief The SQLiteTable class Соединение с БД, управляющее таблицей
 */
class SQLiteTable
{
public:
    /**
     * @brief The ColumnInfo class Метаинформация о столбце таблицы
     */
    struct ColumnInfo
    {
        std::string name;
        ColumnType type         {ColumnType::CT_UNDEFINED};

        bool isPrimaryKey       {false};
        DBCell defaultValue;
        bool canBeNull          {true};

        // Информация о столбце как о внешнем ключе
        std::string referedColumn;
        std::string referenceUpdateAction {"NO ACTION"};
        std::string referenceDeleteAction {"CASCADE"};
    };

    SQLiteTable(SQLiteDatabase& db);

    // ===================================================== //
    // =================== TRANSACTIONS ==================== //
    // ===================================================== //
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    // ===================================================== //
    // ============= TABLE MAIN DATA MANAGEMENT  =========== //
    // ===================================================== //
    /**
     * @brief setTable  Главный метод, указывает таблицу для работы
     * @param tableName
     */
    void setTable(const std::string& tableName);
    std::string getName() const;

    bool isTableExist() const;
    std::string getLastError() const;

    /**
     * @brief create    Создать таблицу в БД
     * @param columns
     * @return          false если таблица уже создана или не удалось её создать
     */
    bool create(const std::list<ColumnInfo>& columns);


    // ===================================================== //
    // ================= COLUMN MANAGEMENT ================= //
    // ===================================================== //
    /**
     * @brief addColumn     Добавить столбец. При добавлении внешнего ключа пересоздаст таблицу (нюанс работы SQLite)
     * @param columnConfig  Информация о столбце
     * @return
     */
    bool addColumn(const ColumnInfo& columnConfig);

    /**
     * @brief getColumns    Получить метаинформацию о столбцах таблицы
     * @return
     */
    std::list<ColumnInfo> getColumns() const;

    /**
     * @brief removeColumn  Удалить столбец таблицы. Пересоздаёт таблицу (нюанс работы SQLite)
     * @param columnName    Название столбца
     * @return
     */
    bool removeColumn(const std::string& columnName);

    /**
     * @brief drop  Удалить таблицу вместе с её данными
     * @return
     */
    bool drop();


    // ===================================================== //
    // ================= ROW DATA MANAGEMENT =============== //
    // ===================================================== //
    /**
     * @brief addRow    Добавить строку в таблицу БД
     * @param rowData
     * @return
     */
    bool addRow(DBRow&& rowData);

    /**
     * @brief addRow        Добавить строку в таблицу в БД, используя названия столбцов
     * @param rowNamedData  Словарь СТОЛБЕЦ - ЗНАЧЕНИЕ_СТОЛБЦА
     * @return
     */
    bool addRow(std::map<std::string, DBCell>&& rowNamedData);

    /**
     * @brief updateRow         Обновить данные в строках таблицы
     * @param rowNamedData      Словарь СТОЛБЕЦ - ЗНАЧЕНИЕ_СТОЛБЦА
     * @param whereCondition    Условие, по которому выбирать целевые строки. Не нужно писать WHERE
     * @return
     */
    bool updateRow(std::map<std::string, DBCell>&& rowNamedData, const std::string& whereCondition = {});

    /**
     * @brief removeRow         Удалить строки таблицы
     * @param whereCondition    Условие, по которому выбирать целевые строки. Не нужно писать WHERE
     * @return
     */
    bool removeRow(const std::string& whereCondition = {});

    /**
     * @brief getRow            Получить строки таблицы
     * @param cols              Столбцы, которые должны быть в строках
     * @param whereCondition    Условие, по которому выбирать результат. Не нужно писать WHERE
     * @param orderCondition    Условие, по которому сортировать результат. Не нужно писать ORDER BY
     * @return
     */
    std::vector<DBRow> getRow(const std::vector<std::string> &cols = {}, const std::string& whereCondition = {}, const std::string& orderCondition = {}) const;

private:
    std::shared_ptr<SQLiteExecutor> m_executor;

    std::string m_name;
    std::list<ColumnInfo> m_columns;

    void initColumns();
};

}
