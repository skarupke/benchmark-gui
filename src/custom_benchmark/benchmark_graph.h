#pragma once

#include "QtGui/QWidget"
#include "custom_benchmark/custom_benchmark.h"
#include "signals/connection.hpp"
#include "QtGui/QImage"

class BenchmarkGraph : public QWidget
{
    GUI_CS_OBJECT(BenchmarkGraph)
public:

    BenchmarkGraph(QWidget * parent = nullptr);

    void AddData(skb::BenchmarkResults * benchmark);
    void RemoveData(skb::BenchmarkResults * benchmark);
    void RemoveAll();

    virtual QSize sizeHint() const override;

    virtual void paintEvent(QPaintEvent*) override;

    void SetNormalizeForMemory(bool value);
    void SetDrawAsPoints(bool value);

    const std::vector<skb::BenchmarkResults *> & GetData() const
    {
        return data;
    }

    void SetXLimit(int limit)
    {
        xlimit = limit;
        lines_dirty = true;
        update();
    }
    int GetXLimit() const
    {
        return xlimit;
    }

    GUI_CS_SIGNAL_1(Public, void RunBenchmarkFirst(skb::BenchmarkResults * benchmark, int argument))
    GUI_CS_SIGNAL_2(RunBenchmarkFirst, benchmark, argument)

private:

    int xlimit = 0;

    std::vector<skb::BenchmarkResults *> data;
    std::vector<sig2::Connection<skb::BenchmarkResults *>> callbacks;
    bool normalize_for_memory = false;
    bool draw_as_points = false;
    QImage lines;
    bool lines_dirty = true;
    struct DrawnPoint
    {
        skb::BenchmarkResults * benchmark;
        int argument;
        double x;
        double y;
        int color;
    };
    std::vector<DrawnPoint> points;
    int xmin = 0;
    int xmax = 0;
    double ymin = 0.0;
    double ymax = 0.0;

    skb::BenchmarkResults * highlighted_benchmark = nullptr;
    int highlighted_argument = 0;
    skb::BenchmarkResults * mouse_press_highlighted = nullptr;
    int mouse_press_highlighted_argument = 0;
    skb::BenchmarkResults * last_emitted = nullptr;

    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *event) override;

    void EmitBenchmark(skb::BenchmarkResults * benchmark, int argument);
    std::string BuildClipboardString(const skb::BenchmarkResults & results) const;
};

