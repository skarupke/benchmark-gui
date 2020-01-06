#include "custom_benchmark/main_gui.hpp"
#include "QAction"
#include "QLabel"
#include <QMessageBox>

BenchmarkMainGui::CategoryCheckboxes::CategoryCheckboxes(interned_string tmp_category)
    : category(std::move(tmp_category)), no_setting_checkbox("None")
{
    parent.setLayout(&layout);
    QLabel * name_label = new QLabel(QString::fromUtf8(category.data(), category.size()));
    layout.addWidget(name_label, 0, 0, Qt::AlignHCenter | Qt::AlignTop);
    for (const auto & setting : skb::Benchmark::AllCategories()[category])
    {
        checkboxes.emplace(setting.first, QString::fromUtf8(setting.first.data(), setting.first.size()));
    }
    size_t layout_index = 0;
    for (auto & added : checkboxes)
    {
        QCheckBox & checkbox = added.second;
        checkbox.setChecked(true);
        layout.addWidget(&checkbox, ++layout_index, 0, Qt::AlignLeft | Qt::AlignTop);
    }
    if (category != skb::BenchmarkCategories::NameIndex() && category != skb::BenchmarkCategories::TypeIndex())
    {
        layout.addWidget(&no_setting_checkbox, checkboxes.size() + 1, 0, Qt::AlignHCenter | Qt::AlignTop);
        no_setting_checkbox.setChecked(true);
    }
    layout.addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), checkboxes.size() + 2, 0, Qt::AlignTop);
}

BenchmarkMainGui::BenchmarkMainGui()
    : run_current("Run Current")
    , reset_current("Delete Current Results")
    , normalize_checkbox("Normalize For Memory")
    , prefer_visible_checkbox("Run Only Visible")
    , profile_mode("Profile Mode")
    , xlimit("0")
{
    setLayout(&layout);
    QGridLayout * checkboxes_layout = new QGridLayout();

    for (const auto & category : skb::Benchmark::AllCategories())
    {
        category_checkboxes.emplace(category.first, category.first);
    }
    size_t layout_index = 0;
    for (auto & added : category_checkboxes)
    {
        CategoryCheckboxes & checkboxes = added.second;
        if (added.first == skb::BenchmarkCategories::NameIndex())
        {
            benchmark_checkbox_area.setWidget(&checkboxes.parent);
            checkboxes.parent.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        }
        else
            checkboxes_layout->addWidget(&checkboxes.parent, 0, ++layout_index, Qt::AlignTop | Qt::AlignLeft);
        QObject::connect(&checkboxes.no_setting_checkbox, &QCheckBox::stateChanged, this, &BenchmarkMainGui::OnCategoryChanged);
        for (auto & checkbox : checkboxes.checkboxes)
        {
            QObject::connect(&checkbox.second, &QCheckBox::stateChanged, this, &BenchmarkMainGui::OnCategoryChanged);
        }
    }
    category_checkboxes.at(skb::BenchmarkCategories::TypeIndex()).checkboxes.at("baseline").setChecked(false);

    QObject::connect(&run_current, &QPushButton::clicked, this, [&](bool)
    {
        RunBenchmarkFirst(BenchmarksForCurrentCheckboxes().benchmarks);
    });
    QObject::connect(&reset_current, &QPushButton::clicked, this, [&](bool)
    {
        if (QMessageBox::warning(this, "Really Delete Results?", "This will delete all results that are currently displayed. Are you sure?", QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes)
            return;
        for (skb::BenchmarkResults * results : graph.GetData())
        {
            results->ClearResults();
        }
    });

    QObject::connect(&normalize_checkbox, &QCheckBox::stateChanged, this, [&](int state)
    {
        graph.SetNormalizeForMemory(state != 0);
    });

    xlimit.setValidator(new QIntValidator());
    xlimit.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QObject::connect(&xlimit, &QLineEdit::textChanged, this, [&](const QString & text)
    {
        graph.SetXLimit(text.toInt());
    });
    xlimit.setText("10000000");

    checkboxes_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding), 0, category_checkboxes.size() + 1, Qt::AlignLeft);
    QWidget * checkboxes_parent_widget = new QWidget();
    checkboxes_parent_widget->setLayout(checkboxes_layout);
    category_checkbox_area.setWidget(checkboxes_parent_widget);
    category_checkbox_area.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    category_checkbox_area.setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    category_checkbox_area.setFrameShape(QFrame::NoFrame);
    category_checkbox_area.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    benchmark_checkbox_area.setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    benchmark_checkbox_area.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    benchmark_checkbox_area.setFrameShape(QFrame::NoFrame);
    benchmark_checkbox_area.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    layout.addWidget(&category_checkbox_area, 0, 0, 1, 2);
    layout.addWidget(&graph, 1, 0);
    QGridLayout * rhs_layout = new QGridLayout();
    rhs_layout->addWidget(&benchmark_checkbox_area, 0, 0);
    rhs_layout->addWidget(&xlimit, 1, 0);
    rhs_layout->addWidget(&run_current, 2, 0);
    rhs_layout->addWidget(&normalize_checkbox, 3, 0);
    rhs_layout->addWidget(&prefer_visible_checkbox, 4, 0);
    rhs_layout->addWidget(&profile_mode, 5, 0);
    rhs_layout->addWidget(&reset_current, 6, 0);
    layout.addLayout(rhs_layout, 1, 1);
}

void BenchmarkMainGui::OnCategoryChanged(int)
{
    EnabledBoxes selection = BenchmarksForCurrentCheckboxes();
    graph.RemoveAll();
    for (skb::BenchmarkResults * result : selection.benchmarks)
    {
        graph.AddData(result);
    }

    for (auto & category : category_checkboxes)
    {
        bool no_setting_visible = selection.checkboxes.find(&category.second.no_setting_checkbox) != selection.checkboxes.end();
        category.second.no_setting_checkbox.setVisible(no_setting_visible);
        int num_visible = no_setting_visible ? 1 : 0;
        for (auto & checkbox : category.second.checkboxes)
        {
            bool visible = selection.checkboxes.find(&checkbox.second) != selection.checkboxes.end();
            checkbox.second.setVisible(visible);
            if (visible)
                ++num_visible;
        }
        category.second.parent.setVisible(num_visible > 1);
    }
}

static const interned_string special_no_setting_checkbox_string = "__special_no_setting_checkbox_is_disabled__";

std::map<interned_string, std::map<interned_string, bool, interned_string::pointer_less>, interned_string::pointer_less> BenchmarkMainGui::GetCheckboxState() const
{
    std::map<interned_string, std::map<interned_string, bool, interned_string::pointer_less>, interned_string::pointer_less> result;
    for (auto & category : category_checkboxes)
    {
        std::map<interned_string, bool, interned_string::pointer_less> & to_insert = result[category.first];
        to_insert[special_no_setting_checkbox_string] = category.second.no_setting_checkbox.isChecked();
        for (auto & checkbox : category.second.checkboxes)
        {
            to_insert[checkbox.first] = checkbox.second.isChecked();
        }
    }
    return result;
}
void BenchmarkMainGui::SetCheckboxState(const std::map<interned_string, std::map<interned_string, bool, interned_string::pointer_less>, interned_string::pointer_less> & categories_and_checkboxes)
{
    std::map<interned_string, std::map<interned_string, bool, interned_string::pointer_less>, interned_string::pointer_less> old_state = GetCheckboxState();
    bool state_is_compatible = std::equal(categories_and_checkboxes.begin(), categories_and_checkboxes.end(), old_state.begin(), old_state.end(), [](const auto & a, const auto & b)
    {
        return a.first == b.first &&
                std::equal(a.second.begin(), a.second.end(), b.second.begin(), b.second.end(), [](const auto & a, const auto & b)
        {
            return a.first == b.first;
        });
    });
    if (!state_is_compatible)
        return;

    for (auto & category : category_checkboxes)
    {
        const std::map<interned_string, bool, interned_string::pointer_less> & state = categories_and_checkboxes.at(category.first);
        category.second.no_setting_checkbox.setChecked(state.at(special_no_setting_checkbox_string));
        for (auto & checkbox : category.second.checkboxes)
        {
            checkbox.second.setChecked(state.at(checkbox.first));
        }
    }
}


template<typename Func>
void BenchmarkMainGui::ForEachNecessaryCheckbox(const skb::BenchmarkCategories & categories, Func && callback)
{
    const auto & categories_map = categories.GetCategories();
    for (auto & category : categories_map)
    {
        QCheckBox * checkbox = &category_checkboxes.at(category.first).checkboxes.at(category.second);
        if (!callback(checkbox))
            return;
    }
    for (auto & category : category_checkboxes)
    {
        if (categories_map.find(category.first) == categories_map.end())
        {
            QCheckBox * checkbox = &category.second.no_setting_checkbox;
            if (!callback(checkbox))
                return;
        }
    }
}


BenchmarkMainGui::EnabledBoxes BenchmarkMainGui::BenchmarksForCurrentCheckboxes()
{
    EnabledBoxes result;
    for (auto & benchmark : skb::Benchmark::AllBenchmarks())
    {
        QCheckBox * enable_checkbox = nullptr;
        bool matched = true;
        ForEachNecessaryCheckbox(*benchmark.second.categories, [&](QCheckBox * checkbox)
        {
            if (checkbox->isChecked())
                return true;
            if (enable_checkbox)
            {
                matched = false;
                return false;
            }
            else
            {
                enable_checkbox = checkbox;
                return true;
            }
        });
        if (matched)
        {
            if (enable_checkbox)
                result.checkboxes.insert(enable_checkbox);
            else
            {
                result.benchmarks.push_back(&benchmark.second);
                ForEachNecessaryCheckbox(*benchmark.second.categories, [&](QCheckBox * checkbox)
                {
                    result.checkboxes.insert(checkbox);
                    return true;
                });
            }
        }
    }
    return result;
}

