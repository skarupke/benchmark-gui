#pragma once

#include <QGridLayout>
#include <QComboBox>
#include <QCheckBox>
#include "custom_benchmark/benchmark_graph.h"
#include <QPushButton>
#include <QScrollArea>
#include <QLineEdit>

class BenchmarkMainGui : public QWidget
{
    GUI_CS_OBJECT(BenchmarkMainGui)
public:
    BenchmarkMainGui();

    GUI_CS_SIGNAL_1(Public, void RunBenchmarkFirst(const std::vector<skb::BenchmarkResults *> & to_run))
    GUI_CS_SIGNAL_2(RunBenchmarkFirst, to_run)

    BenchmarkGraph & GetGraph()
    {
        return graph;
    }
    bool ShouldPreferVisible() const
    {
        return prefer_visible_checkbox.isChecked();
    }
    bool ProfileMode() const
    {
        return profile_mode.isChecked();
    }

    std::map<std::string, std::map<std::string, bool>> GetCheckboxState() const;
    void SetCheckboxState(const std::map<std::string, std::map<std::string, bool>> & categories_and_checkboxes);

private:
    QScrollArea benchmark_checkbox_area;
    QScrollArea category_checkbox_area;
    QGridLayout layout;

    QPushButton run_current;
    QPushButton reset_current;
    QCheckBox normalize_checkbox;
    QCheckBox prefer_visible_checkbox;
    QCheckBox profile_mode;
    QLineEdit xlimit;

    BenchmarkGraph graph;

    std::map<std::string, size_t> all_lines;

    struct CategoryCheckboxes
    {
        CategoryCheckboxes(std::string category);

        std::string category;
        QWidget parent;
        QGridLayout layout;
        std::map<std::string, QCheckBox> checkboxes;
        QCheckBox no_setting_checkbox;
    };

    std::map<std::string, CategoryCheckboxes> category_checkboxes;

    void OnCategoryChanged(int);

    struct EnabledBoxes
    {
        std::vector<skb::BenchmarkResults *> benchmarks;
        std::set<QCheckBox *> checkboxes;
    };

    EnabledBoxes BenchmarksForCurrentCheckboxes();
    template<typename Func>
    void ForEachNecessaryCheckbox(const skb::BenchmarkCategories & categories, Func && callback);
};

