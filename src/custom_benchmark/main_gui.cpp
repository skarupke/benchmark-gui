#include "custom_benchmark/main_gui.hpp"
#include <QAction>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QDoubleValidator>
#include <QDebug>
#include "custom_benchmark/profile_mode.hpp"

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
    : rhs_layout(new QGridLayout())
    , add_benchmarks("Add Benchmark Executable")
    , reset_current("Delete Current Results")
    , normalize_checkbox("Normalize For Memory")
    , draw_points_checkbox("Draw as Points")
    , profile_mode("Profile Mode")
    , xlimit_label("x-axis limit:")
    , xlimit("0")
{
    setLayout(&layout);

    auto init_checkboxes = [this] {
        category_checkboxes.clear();
        checkboxes_widget = std::make_unique<QWidget>();
        for (const auto & category : skb::Benchmark::AllCategories())
        {
            category_checkboxes.emplace(category.first, category.first);
        }
        QGridLayout * checkboxes_layout = new QGridLayout();
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
        auto types = category_checkboxes.find(skb::BenchmarkCategories::TypeIndex());
        if (types != category_checkboxes.end()) {
            auto baseline = types->second.checkboxes.find("baseline");
            if (baseline != types->second.checkboxes.end()) {
                baseline->second.setChecked(false);
            }
        }
        checkboxes_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding), 0, category_checkboxes.size() + 1, Qt::AlignLeft);
        checkboxes_widget->setLayout(checkboxes_layout);
        category_checkbox_area.setWidget(checkboxes_widget.get());
        layout.addWidget(&category_checkbox_area, 0, 0, 1, 2);
        rhs_layout->addWidget(&benchmark_checkbox_area, rhs_checkbox_row, 0, 1, 2);
    };
    init_checkboxes();

    QObject::connect(&add_benchmarks, &QPushButton::clicked, this, [this, init_checkboxes](bool){
        QString result = QFileDialog::getOpenFileName(this, "Select an executable with benchmarks in it", ".", "Executables (*.exe)");
        if (result.isEmpty())
            return;
        QByteArray as_utf8 = result.toUtf8();
        std::string as_string(as_utf8.begin(), as_utf8.end());
        if (auto error = skb::LoadAllBenchmarksFromFile(as_string)) {
            std::string error_message = "There was an error loading the benchmarks:\n" + *error;
            QMessageBox::critical(this, "Failed to Load Benchmarks", QString::fromUtf8(error_message.data(), error_message.size()), QMessageBox::Ok);
        } else {
            auto checkbox_state = GetCheckboxState();
            init_checkboxes();
            SetCheckboxState(checkbox_state);
            NewFileLoaded(interned_string(as_string));
        }
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
    QObject::connect(&draw_points_checkbox, &QCheckBox::stateChanged, this, [&](int state)
    {
        graph.SetDrawAsPoints(state != 0);
    });
    QObject::connect(&profile_mode, &QCheckBox::stateChanged, this, [&](int state)
    {
        if (state == 0) {
            skb::DisableProfileMode();
        }
    });

    xlimit.setValidator(new QDoubleValidator());
    xlimit.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QObject::connect(&xlimit, &QLineEdit::textChanged, this, [&](const QString & text)
    {
        if (is_formatting_xlimit) {
            return;
        }
        QByteArray utf8 = text.toUtf8();
        std::string unformatted(utf8.data(), utf8.length());
        unformatted.erase(std::remove(unformatted.begin(), unformatted.end(), ','), unformatted.end());
        int64_t new_xlimit = std::stoll(unformatted);
        graph.SetXLimit(new_xlimit);
        QString new_text = readable_xvalue(new_xlimit);
        int cursor = xlimit.cursorPosition() + new_text.length() - text.length();
        is_formatting_xlimit = true;
        xlimit.setText(new_text);
        xlimit.setCursorPosition(cursor);
        is_formatting_xlimit = false;
    });
    xlimit.setText("1000000");

    category_checkbox_area.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    category_checkbox_area.setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    category_checkbox_area.setFrameShape(QFrame::NoFrame);
    category_checkbox_area.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    benchmark_checkbox_area.setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    benchmark_checkbox_area.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    benchmark_checkbox_area.setFrameShape(QFrame::NoFrame);
    benchmark_checkbox_area.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    layout.addWidget(&category_checkbox_area, 0, 0, 1, 2);
    layout.addWidget(&graph, 1, 0);
    int row = 0;
    rhs_layout->addWidget(&add_benchmarks, row++, 0, 1, 2);
    rhs_checkbox_row = row;
    rhs_layout->addWidget(&benchmark_checkbox_area, row++, 0, 1, 2);
    rhs_layout->addWidget(&xlimit_label, row, 0);
    rhs_layout->addWidget(&xlimit, row++, 1);
    rhs_layout->addWidget(&normalize_checkbox, row++, 0, 1, 2);
    rhs_layout->addWidget(&draw_points_checkbox, row++, 0, 1, 2);
    rhs_layout->addWidget(&profile_mode, row++, 0, 1, 2);
    rhs_layout->addWidget(&reset_current, row++, 0, 1, 2);
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

BenchmarkMainGui::checkbox_state BenchmarkMainGui::GetCheckboxState() const
{
    checkbox_state result;
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
bool BenchmarkMainGui::CheckboxStateMatches(const checkbox_state & state)
{
    checkbox_state old_state = GetCheckboxState();
    return std::equal(state.begin(), state.end(), old_state.begin(), old_state.end(), [](const auto & a, const auto & b)
    {
        return a.first == b.first &&
                std::equal(a.second.begin(), a.second.end(), b.second.begin(), b.second.end(), [](const auto & a, const auto & b)
        {
            return a.first == b.first;
        });
    });
}
void BenchmarkMainGui::SetCheckboxState(const checkbox_state & categories_and_checkboxes)
{
    for (auto & category : category_checkboxes)
    {
        auto found_category = categories_and_checkboxes.find(category.first);
        if (found_category == categories_and_checkboxes.end())
            continue;
        category.second.no_setting_checkbox.setChecked(found_category->second.at(special_no_setting_checkbox_string));
        for (auto & checkbox : category.second.checkboxes)
        {
            auto found_checkbox = found_category->second.find(checkbox.first);
            if (found_checkbox == found_category->second.end())
                continue;
            checkbox.second.setChecked(found_checkbox->second);
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
            else if (enable_checkbox)
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

