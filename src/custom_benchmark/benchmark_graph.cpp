#include "custom_benchmark/benchmark_graph.h"
#include <algorithm>
#include <QtGui/QPainter>
#include <cmath>
#include "math/my_math.hpp"
#include <QtGui/QtEvents>
#include <QtGui/QAction>
#include <QClipboard>
#include <QApplication>
#include <QToolTip>

BenchmarkGraph::BenchmarkGraph(QWidget * parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
    QAction * copy_to_clipboard_action = new QAction("Copy to clipboard", this);
    addAction(copy_to_clipboard_action);
    QObject::connect(copy_to_clipboard_action, &QAction::triggered, this, [&](bool)
    {
        if (mouse_press_highlighted)
        {
            std::string for_clipboard = BuildClipboardString(*mouse_press_highlighted, Without_Error_Bars);
            QApplication::clipboard()->setText(QString::fromUtf8(for_clipboard.c_str()));
        }
    });
    QAction * copy_with_error_bars_action = new QAction("Copy to clipboard with error bars", this);
    addAction(copy_with_error_bars_action);
    QObject::connect(copy_with_error_bars_action, &QAction::triggered, this, [&](bool)
    {
        if (mouse_press_highlighted)
        {
            std::string for_clipboard = BuildClipboardString(*mouse_press_highlighted, With_Error_Bars);
            QApplication::clipboard()->setText(QString::fromUtf8(for_clipboard.c_str()));
        }
    });
}

void BenchmarkGraph::EmitBenchmark(skb::BenchmarkResults * benchmark, int argument)
{
    if (benchmark)
    {
        RunBenchmarkFirst(benchmark, argument);
    }
    last_emitted = benchmark;
}

std::string BenchmarkGraph::BuildClipboardString(const skb::BenchmarkResults & results, ClipboardStringType type) const
{
    std::lock_guard<std::mutex> lock(results.results_mutex);
    skb::BenchmarkResults * baseline = results.baseline_results;
    std::string clipboard_string;
    std::vector<double> all_results;
    for (const auto & run : results.results)
    {
        if (run.first <= 0 || run.second.empty())
            continue;
        if (xlimit > 0 && run.first > xlimit)
            continue;
        clipboard_string += std::to_string(run.first);
        clipboard_string += ';';
        double nanoseconds = run.second.front().GetNanosecondsPerItem(baseline);
        clipboard_string += std::to_string(nanoseconds);
        if (type == With_Error_Bars)
        {
            all_results.clear();
            for (const skb::RunResults & result : run.second)
            {
                all_results.push_back(result.GetNanosecondsPerItem(baseline));
            }
            size_t quarter = all_results.size() / 4;
            auto percentile_25 = all_results.begin() + quarter;
            std::nth_element(all_results.begin(), percentile_25, all_results.end());
            auto percentile_75 = all_results.end() - std::max(size_t(1), quarter);
            if (percentile_75 != percentile_25)
                std::nth_element(percentile_25 + 1, percentile_75, all_results.end());
            clipboard_string += ';';
            clipboard_string += std::to_string(nanoseconds - *percentile_25);
            clipboard_string += ';';
            clipboard_string += std::to_string(*percentile_75 - nanoseconds);
        }
        clipboard_string += '\n';
    }
    return clipboard_string;
}

void BenchmarkGraph::mousePressEvent(QMouseEvent * event)
{
    mouse_press_highlighted = highlighted_benchmark;
    mouse_press_highlighted_argument = highlighted_argument;
    QWidget::mousePressEvent(event);
}

void BenchmarkGraph::mouseReleaseEvent(QMouseEvent * event)
{
    if (event->button() == Qt::LeftButton)
    {
        for (int i = 0; i < 2; ++i)
            EmitBenchmark(highlighted_benchmark, highlighted_argument);
    }
    QWidget::mouseReleaseEvent(event);
}

void BenchmarkGraph::mouseMoveEvent(QMouseEvent * event)
{
    update();
    if (event->buttons() & Qt::LeftButton)
    {
        if (last_emitted != highlighted_benchmark)
            EmitBenchmark(highlighted_benchmark, highlighted_argument);
    }
    if (highlighted_benchmark)
    {
        skb::BenchmarkResults * baseline = highlighted_benchmark->baseline_results;
        QString tooltip_string = QString::fromUtf8(highlighted_benchmark->categories->CategoriesString().c_str());
        tooltip_string += '\n';
        tooltip_string += QString::number(highlighted_argument);
        tooltip_string += '\n';
        tooltip_string += QString::number(highlighted_benchmark->results[highlighted_argument].front().GetNanosecondsPerItem(baseline), 'f', 2);
        QToolTip::showText(event->globalPos(), tooltip_string);
    }
    else
        QToolTip::hideText();
    QWidget::mouseMoveEvent(event);
}


void BenchmarkGraph::AddData(skb::BenchmarkResults * benchmark)
{
    data.push_back(benchmark);
    callbacks.push_back(benchmark->results_added_signal.map([this](skb::BenchmarkResults *)
    {
        lines_dirty = true;
        update();
    }));
    lines_dirty = true;
    update();
}
void BenchmarkGraph::RemoveData(skb::BenchmarkResults * benchmark)
{
    auto found = std::find(data.begin(), data.end(), benchmark);
    if (found == data.end())
        return;
    callbacks.erase(callbacks.begin() + std::distance(data.begin(), found));
    data.erase(found);
    lines_dirty = true;
    update();
}
void BenchmarkGraph::RemoveAll()
{
    callbacks.clear();
    data.clear();
    lines_dirty = true;
    update();
}
void BenchmarkGraph::SetNormalizeForMemory(bool value)
{
    normalize_for_memory = value;
    lines_dirty = true;
    update();
}
void BenchmarkGraph::SetDrawAsPoints(bool value)
{
    draw_as_points = value;
    lines_dirty = true;
    update();
}

QSize BenchmarkGraph::sizeHint() const
{
    return { 800, 500 };
}

struct GraphLabelPos
{
    double min = 0.0;
    double max = 0.0;
    double add_step = 0.0;
    double multiply_step = 1.0;

    double step(double value) const
    {
        return (value + add_step) * multiply_step;
    }
};

GraphLabelPos GetXStep(int xmin, int xmax, int width)
{
    int range = xmax - xmin;
    int num_labels = std::max(2, width / 100);
    int64_t two_power = 1;
    while (int64_t(2) << (two_power * num_labels) < range)
        ++two_power;
    double power_of_two_multiplier = 1 << two_power;
    double power_of_two_min = 1.0;
    while (power_of_two_min * 2.0 <= xmin)
        power_of_two_min *= 2.0;
    double power_of_two_max = power_of_two_min * power_of_two_multiplier;
    while (power_of_two_max * power_of_two_multiplier <= xmax)
        power_of_two_max *= power_of_two_multiplier;
    return { power_of_two_min, power_of_two_max, 0.0, power_of_two_multiplier };
}
double PowerOfTenValue(double ideal)
{
    double rounded = 1.0;
    if (rounded > ideal)
    {
        while (rounded * 0.5f > ideal)
        {
            rounded *= 0.5f;
        }
        return rounded;
    }
    else
    {
        for (;;)
        {
            if (rounded * 2.0 <= ideal)
                rounded *= 2.0;
            else
                return rounded;
            if (rounded * 2.5 <= ideal)
                rounded *= 2.5;
            else
                return rounded;
            if (rounded * 2.0 <= ideal)
                rounded *= 2.0;
            else
                return rounded;
        }
    }
}

QString readable_yvalue(double yvalue)
{
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%g", yvalue);
    return QString::fromUtf8(buffer);
}
QString readable_xvalue(double xvalue)
{
    CHECK_FOR_PROGRAMMER_ERROR(xvalue >= 1.0);
    char buffer[128];
    char * to_write = buffer + 128;
    *--to_write = '\0';
    int count = 4;
    for (int64_t x = static_cast<int64_t>(xvalue); x; x /= 10) {
        if (--count == 0) {
            *--to_write = ',';
            count = 3;
        }
        *--to_write = '0' + x % 10;
    }
    return QString::fromUtf8(to_write);
}

GraphLabelPos GetYStep(double ymin, double ymax, int height)
{
    double range = ymax - ymin;
    int num_labels = std::max(2, height / 100);
    double ideal_add = range / num_labels;
    double rounded_add = PowerOfTenValue(ideal_add);
    double rounded_min = rounded_add * static_cast<int>(ymin / rounded_add);
    double rounded_max = rounded_min + rounded_add;
    rounded_max += rounded_add * static_cast<int>((ymax - rounded_max) / rounded_add);
    return { rounded_min, rounded_max, rounded_add, 1.0 };
}

void BenchmarkGraph::paintEvent(QPaintEvent *)
{
    QSize overall_size = size();
    float line_width = std::max(1.0f, overall_size.height() * 0.0025f);
    QPainter my_painter(this);

    int text_height = my_painter.fontMetrics().height();

    static constexpr int label_width = 50;
    int label_height = 2 * text_height;
    static constexpr int tick_length = 5;
    QSize lines_size(overall_size.width() - label_width, overall_size.height() - label_height);

    double log_xmin = std::log(xmin);
    double log_xrange = std::log(xmax) - log_xmin;

    auto position_x = [&](double xvalue)
    {
        double x_percent = (std::log(xvalue) - log_xmin) / log_xrange;
        return label_width + x_percent * lines_size.width();
    };
    auto position_y = [&](double yvalue)
    {
        double y_percent = 1.0f - (yvalue - ymin) / (ymax - ymin);
        return y_percent * lines_size.height();
    };

    auto yval_from_result = [&](const skb::RunResults & median, skb::BenchmarkResults * baseline)
    {
        double time = median.GetNanosecondsPerItem(baseline);
        if (normalize_for_memory && median.num_bytes_used > 0 && median.argument > 0)
        {
            double memory_per_item = median.num_bytes_used / static_cast<double>(median.argument);
            time *= memory_per_item;
        }
        return time;
    };

    static const QColor colors[12] =
    {
        QColor(0x1f, 0x78, 0xb4), // blue
        QColor(0x33, 0xa0, 0x2c), // green
        QColor(0xe3, 0x1a, 0x1c), // red
        QColor(0xff, 0x7f, 0x00), // orange
        QColor(0x6a, 0x3d, 0x9a), // purple
        QColor(0xb1, 0x59, 0x28), // brown
        QColor(0xa6, 0xce, 0xe3), // light blue
        QColor(0xb2, 0xdf, 0x8a), // light green
        QColor(0xfb, 0x9a, 0x99), // light red
        QColor(0xfd, 0xbf, 0x6f), // light orange
        QColor(0xca, 0xb2, 0xd6), // light purple
        QColor(0x46, 0x33, 0x05), // dark brown
    };

    if (lines_dirty || lines.size() != overall_size)
    {
        lines_dirty = false;
        if (lines.size() != overall_size)
            lines = QImage(overall_size, QImage::Format_ARGB32);
        xmin = std::numeric_limits<int>::max();
        xmax = std::numeric_limits<int>::lowest();
        ymin = 0.0;
        ymax = std::numeric_limits<double>::lowest();

        for (skb::BenchmarkResults * benchmark : data)
        {
            std::lock_guard<std::mutex> lock(benchmark->results_mutex);
            skb::BenchmarkResults * baseline = benchmark->baseline_results;
            for (const auto & run : benchmark->results)
            {
                if (run.first <= 0 || run.second.empty())
                    continue;
                if (xlimit > 0 && run.first > xlimit)
                    continue;
                xmin = std::min(xmin, run.first);
                xmax = std::max(xmax, run.first);
                double time = yval_from_result(run.second.front(), baseline);
                ymin = std::min(ymin, time);
                ymax = std::max(ymax, time);
            }
        }
        QPainter graph_painter(&lines);
        graph_painter.eraseRect(0, 0, overall_size.width(), overall_size.height());
        graph_painter.setBackground(QBrush(QColor(255, 255, 255)));
        graph_painter.setRenderHint(QPainter::Antialiasing, true);

        if (xmin >= xmax || ymin >= ymax)
            return;
        log_xmin = std::log(xmin);
        log_xrange = std::log(xmax) - log_xmin;

        graph_painter.fillRect(QRectF(QPointF(position_x(xmin), position_y(ymin)), QPointF(position_x(xmax), position_y(ymax))), QColor(255, 255, 255));

        GraphLabelPos xlabels = GetXStep(xmin, xmax, lines_size.width());
        GraphLabelPos ylabels = GetYStep(ymin, ymax, lines_size.height());

        for (double x = xlabels.min; x <= xlabels.max; x = xlabels.step(x))
        {
            double screen_x = position_x(x);
            double screen_y = position_y(ymin);
            graph_painter.drawLine(QPointF(screen_x, screen_y), QPointF(screen_x, screen_y + tick_length));
            QRectF bounding_rect(QPointF(screen_x - 100.0, screen_y + tick_length), QPointF(screen_x + 100.0, size().height()));
            graph_painter.drawText(bounding_rect, Qt::AlignTop | Qt::AlignHCenter, readable_xvalue(x));
        }
        for (double y = ylabels.min; y <= ylabels.max; y = ylabels.step(y))
        {
            double screen_x = position_x(xmin);
            double screen_y = position_y(y);
            graph_painter.drawLine(QPointF(screen_x, screen_y), QPointF(screen_x - tick_length, screen_y));
            QRectF bounding_rect(QPointF(0.0, screen_y - 2.0 * text_height), QPointF(screen_x - tick_length - 2, screen_y + 2.0 * text_height));
            graph_painter.drawText(bounding_rect, Qt::AlignRight | Qt::AlignVCenter, readable_yvalue(y));
        }

        points.clear();
        int color_choice = 0;
        std::vector<QPointF> benchmark_points;
        for (skb::BenchmarkResults * benchmark : data)
        {
            std::lock_guard<std::mutex> lock(benchmark->results_mutex);
            skb::BenchmarkResults * baseline = benchmark->baseline_results;

            if (draw_as_points)
            {
                benchmark_points.clear();
                auto begin = benchmark->results.begin();
                auto end = benchmark->results.end();
                for (auto it = begin; it != end; ++it)
                {
                    if (it->first <= 0 || it->second.empty())
                        continue;
                    auto next = std::next(it);
                    double jitter = 0.0;
                    double jitter_amount = 0.4;
                    if (it == begin) {
                        if (next != end) {
                            jitter = static_cast<double>(next->first) / it->first;
                        }
                    }
                    else if (next != end && next->first < xlimit)
                    {
                        if (color_choice & 1) {
                            jitter = std::prev(it)->first / static_cast<double>(it->first);
                        }
                        else {
                            jitter = static_cast<double>(next->first) / it->first;
                        }
                    }
                    else {
                        jitter = std::prev(it)->first / static_cast<double>(it->first);
                    }
                    double jitter_fraction = color_choice / static_cast<double>(std::extent<decltype(colors)>::value);
                    jitter = std::pow(jitter, jitter_amount * jitter_fraction);
                    for (const skb::RunResults & result : it->second)
                    {
                        int xvalue = result.argument;
                        double yvalue = yval_from_result(result, baseline);
                        QPointF point(position_x(xvalue * jitter), position_y(yvalue));
                        benchmark_points.push_back(point);
                        points.push_back({benchmark, result.argument, point.x(), point.y(), color_choice});
                    }
                }
                if (benchmark_points.empty())
                    continue;
                QColor color = colors[color_choice];
                color.setAlpha(127);
                QPen pen(color);
                pen.setWidthF(line_width * 4.0f);
                graph_painter.setPen(pen);
                graph_painter.drawPoints(benchmark_points.data(), static_cast<int>(benchmark_points.size()));
            }
            else
            {
                QPainterPath path;

                bool first = true;
                for (auto & range : benchmark->results)
                {
                    if (range.first <= 0 || range.second.empty())
                        continue;
                    auto median = range.second.begin();

                    int xvalue = median->argument;
                    double yvalue = yval_from_result(*median, baseline);
                    QPointF point(position_x(xvalue), position_y(yvalue));
                    if (first)
                    {
                        first = false;
                        path.moveTo(point);
                    }
                    else
                        path.lineTo(point);
                    points.push_back({benchmark, median->argument, point.x(), point.y(), color_choice});
                }
                if (first)
                    continue;
                QPen pen(colors[color_choice]);
                pen.setWidthF(line_width);
                graph_painter.setPen(pen);
                graph_painter.drawPath(path);
            }
            color_choice = (color_choice + 1) % std::extent<decltype(colors)>::value;
        }
    }

    my_painter.drawImage(0, 0, lines);
    highlighted_benchmark = nullptr;
    highlighted_argument = 0;
    int highlighted_color = 0;
    float closest_point_distance = squared(20.0f);
    QPointF closest_point;
    QPoint cursor_pos = mapFromGlobal(QCursor::pos());
    for (const DrawnPoint & point : points)
    {
        float distance = squared(point.x - cursor_pos.x()) + squared(point.y - cursor_pos.y());
        if (distance < closest_point_distance)
        {
            highlighted_argument = point.argument;
            highlighted_benchmark = point.benchmark;
            closest_point_distance = distance;
            closest_point = QPointF(point.x, point.y);
            highlighted_color = point.color;
        }
    }
    if (highlighted_benchmark)
    {
        QPainterPath path;
        float mark_size = 3.0f * line_width;
        path.moveTo(closest_point.x(), closest_point.y() + mark_size);
        path.lineTo(closest_point.x() + mark_size, closest_point.y());
        path.lineTo(closest_point.x(), closest_point.y() - mark_size);
        path.lineTo(closest_point.x() - mark_size, closest_point.y());
        my_painter.fillPath(path, QBrush(Qt::black));
        setContextMenuPolicy(Qt::ActionsContextMenu);

        double x_zero = position_x(xmin);
        double y_zero = position_y(ymin);
        my_painter.drawLine(QPointF(closest_point.x(), y_zero), QPointF(closest_point.x(), y_zero + tick_length));
        my_painter.drawLine(QPointF(x_zero, closest_point.y()), QPointF(x_zero - tick_length, closest_point.y()));

        bool has_previous = false;
        double last_mean = 0.0;
        double last_stddev = 0.0;
        int last_key = 0;
        skb::BenchmarkResults * baseline = highlighted_benchmark->baseline_results;
        for (const auto & one_point_result : highlighted_benchmark->results)
        {
            if (one_point_result.second.size() < 2)
            {
                has_previous = false;
                continue;
            }
            double mean = 0.0;
            double std_dev = 0.0;
            for (const skb::RunResults & result : one_point_result.second)
            {
                double yval = yval_from_result(result, baseline);
                mean += yval;
            }
            mean /= one_point_result.second.size();
            for (const skb::RunResults & result : one_point_result.second)
                std_dev += squared(mean - yval_from_result(result, baseline));
            std_dev /= one_point_result.second.size();
            std_dev = std::sqrt(std_dev);
            if (has_previous)
            {
                double last_min = position_y(last_mean - last_stddev);
                double last_max = position_y(last_mean + last_stddev);
                double min = position_y(mean - std_dev);
                double max = position_y(mean + std_dev);
                double last_x = position_x(last_key);
                double x = position_x(one_point_result.first);
                QColor color = colors[highlighted_color];
                color.setAlpha(color.alpha() / 8);
                my_painter.setPen(Qt::NoPen);
                QPainterPath path;
                CHECK_FOR_PROGRAMMER_ERROR(!std::isnan(last_x) && !std::isnan(last_min) && !std::isnan(last_max)
                                           && !std::isnan(x) && !std::isnan(min) && !std::isnan(max));
                path.moveTo(last_x, last_min);
                path.lineTo(last_x, last_max);
                path.lineTo(x, max);
                path.lineTo(x, min);
                my_painter.fillPath(path, QBrush(color));
            }
            has_previous = true;
            last_mean = mean;
            last_stddev = std_dev;
            last_key = one_point_result.first;
        }
    }
    else
    {
        setContextMenuPolicy(Qt::NoContextMenu);
        last_emitted = nullptr;
    }
}

#include "test/include_test.hpp"

TEST(benchmark_graph, x_formatting)
{
    ASSERT_EQ("5", readable_xvalue(5.0));
    ASSERT_EQ("50", readable_xvalue(50.0));
    ASSERT_EQ("100", readable_xvalue(100.0));
    ASSERT_EQ("9,102", readable_xvalue(9102.0));
    ASSERT_EQ("1,048,576", readable_xvalue(1024.0 * 1024.0));
    ASSERT_EQ("1,073,741,824", readable_xvalue(1024.0 * 1024.0 * 1024.0));
}
