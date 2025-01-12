#pragma once

#include <QGridLayout>
#include <QComboBox>
#include <QCheckBox>
#include "custom_benchmark/benchmark_graph.h"
#include <QPushButton>
#include <QScrollArea>
#include <QLineEdit>
#include <QLabel>
#include <memory>

class BenchmarkMainGui : public QWidget
{
    GUI_CS_OBJECT(BenchmarkMainGui)
public:
    BenchmarkMainGui();

    GUI_CS_SIGNAL_1(Public, void NewFileLoaded(interned_string filename))
    GUI_CS_SIGNAL_2(NewFileLoaded, filename)

    BenchmarkGraph & GetGraph()
    {
        return graph;
    }
    bool ProfileMode() const
    {
        return profile_mode.isChecked();
    }

    using checkbox_state = std::map<interned_string, std::map<interned_string, bool, interned_string::pointer_less>, interned_string::pointer_less>;

    checkbox_state GetCheckboxState() const;
    bool CheckboxStateMatches(const checkbox_state & state);
    void SetCheckboxState(const checkbox_state & categories_and_checkboxes);

private:
    QScrollArea benchmark_checkbox_area;
    QScrollArea category_checkbox_area;
    QGridLayout * rhs_layout; // owned by [layout]
    QGridLayout layout;
    int rhs_checkbox_row = 0;
    std::unique_ptr<QWidget> checkboxes_widget;

    QPushButton add_benchmarks;
    QPushButton reset_current;
    QCheckBox normalize_checkbox;
    QCheckBox draw_points_checkbox;
    QCheckBox profile_mode;
    QLabel xlimit_label;
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

