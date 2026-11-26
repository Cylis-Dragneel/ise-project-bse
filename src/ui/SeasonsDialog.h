#ifndef SEASONS_DIALOG_H
#define SEASONS_DIALOG_H

#include <QDialog>

class SeasonService;
class QTableWidget;

// Manager's Seasons.txt editor (requirement 14). All changes go through
// SeasonService, so overlap rules and file persistence are enforced there.
class SeasonsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SeasonsDialog(SeasonService *seasons, QWidget *parent = nullptr);

private:
    SeasonService *seasons_;
    QTableWidget *table_ = nullptr;

    void refresh();
    void addSeason();
    void editSeason(int row);
    void toggleActive(int row);
    void removeSeason(int row);
};

#endif // SEASONS_DIALOG_H