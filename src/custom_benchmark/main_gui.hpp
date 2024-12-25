#pragma once

#include <QGridLayout>
#include <QComboBox>
#include <QCheckBox>
#include "custom_benchmark/benchmark_graph.h"
#include <QPushButton>
#include <QScrollArea>
#include <QLineEdit>
#include <memory>

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

    std::map<interned_string, std::map<interned_string, bool, interned_string::pointer_less>, interned_string::pointer_less> GetCheckboxState() const;
    void SetCheckboxState(const std::map<interned_string, std::map<interned_string, bool, interned_string::pointer_less>, interned_string::pointer_less> & categories_and_checkboxes);

private:
    QScrollArea benchmark_checkbox_area;
    QScrollArea category_checkbox_area;
    QGridLayout layout;
    std::unique_ptr<QWidget> checkboxes_widget;

    QPushButton add_benchmarks;
    QPushButton run_current;
    QPushButton reset_current;
    QCheckBox normalize_checkbox;
    QCheckBox draw_points_checkbox;
    QCheckBox prefer_visible_checkbox;
    QCheckBox profile_mode;
    QLineEdit xlimit;

    BenchmarkGraph graph;

    struct CategoryCheckboxes
    {
        CategoryCheckboxes(interned_string category);

        interned_string category;
        QWidget parent;
        QGridLayout layout;
        std::map<interned_string, QCheckBox, interned_string::string_less> checkboxes;
        QCheckBox no_setting_checkbox;
    };

    std::map<interned_string, CategoryCheckboxes, interned_string::string_less> category_checkboxes;

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

