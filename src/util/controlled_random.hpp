#pragma once

#include <vector>
#include <random>
#include <algorithm>
#include <cassert>

template<typename T>
inline T lerp(T a, T b, float t)
{
    T dist = b - a;
    return a + t * dist;
}

template<typename It, typename Compare>
void heap_top_updated(It begin, It end, Compare && compare)
{
    using std::swap;
    std::ptrdiff_t num_items = end - begin;
    for (std::ptrdiff_t current = 0;;)
    {
        std::ptrdiff_t child_to_update = current * 2 + 1;
        if (child_to_update >= num_items)
            break;
        std::ptrdiff_t second_child = child_to_update + 1;
        if (second_child < num_items && compare(begin[child_to_update], begin[second_child]))
            child_to_update = second_child;
        if (!compare(begin[current], begin[child_to_update]))
            break;
        swap(begin[current], begin[child_to_update]);
        current = child_to_update;
    }
}
template<typename It>
void heap_top_updated(It begin, It end)
{
    return heap_top_updated(begin, end, std::less<>());
}

class WeightedDistribution
{
    struct Weight
    {
        explicit Weight(float frequency, size_t original_index)
            : next_event_time(frequency)
            , average_time_between_events(frequency)
            , original_index(original_index)
        {
        }

        float next_event_time;
        float average_time_between_events;
        size_t original_index;
        size_t cycle_count = 0;
    };
    struct CompareByNextTime
    {
        bool operator()(const Weight & l, const Weight & r) const
        {
            if (l.cycle_count < r.cycle_count)
                return false;
            else if (l.cycle_count > r.cycle_count)
                return true;
            else
                return l.next_event_time > r.next_event_time;
        }
    };

    std::vector<Weight> weights;

public:

    WeightedDistribution()
    {
    }

    WeightedDistribution(std::initializer_list<float> il)
    {
        weights.reserve(il.size());
        for (float w : il)
            add_weight(w);
    }

    void add_weight(float w)
    {
        weights.emplace_back(1.0f / w, weights.size());
    }

    size_t num_weights() const
    {
        return weights.size();
    }

    // you need to call this once after adding all the weights to this WeightedDistribution
    // otherwise the first couple of picks will always be deterministic.
    template<typename Random>
    void initialize_randomness(Random & randomness)
    {
        for (Weight & w : weights)
        {
            w.next_event_time = std::uniform_real_distribution<float>(0.0f, w.average_time_between_events)(randomness);
        }
        std::make_heap(weights.begin(), weights.end(), CompareByNextTime());
    }

    bool use_exponential = false;
    float lerp_from_random_to_deterministic = 0.25f;

    // use this to pick a random item. it will give the distribution that you asked for
    // but try to not repeat the same item too often, or to let too much time pass
    // since an item was picked before it gets picked again.
    template<typename Random>
    size_t pick_random(Random & randomness)
    {
        Weight & picked = weights.front();
        size_t result = picked.original_index;
        float to_add_deterministic = picked.average_time_between_events;
        if (use_exponential)
        {
            float to_add_exponential = std::exponential_distribution<float>(1.0f / picked.average_time_between_events)(randomness);
            picked.next_event_time += lerp(to_add_exponential, to_add_deterministic, lerp_from_random_to_deterministic);
        }
        else
        {
            float to_add_random = std::uniform_real_distribution<float>(0.0f, picked.average_time_between_events)(randomness);
            picked.next_event_time += lerp(to_add_random, to_add_deterministic, lerp_from_random_to_deterministic);
        }
        static constexpr float cycle_size = 4.0f;
        while (picked.next_event_time >= cycle_size)
        {
            picked.next_event_time -= cycle_size;
            ++picked.cycle_count;
        }
        heap_top_updated(weights.begin(), weights.end(), CompareByNextTime());
        return result;
    }
};

class WeightedDistributionSimple
{
    struct Weight
    {
        Weight(float frequency, size_t original_index)
            : next_event_time(frequency)
            , average_time_between_events(frequency)
            , original_index(original_index)
        {
        }

        float next_event_time;
        float average_time_between_events;
        size_t original_index;
    };
    struct CompareByNextTime
    {
        bool operator()(const Weight & l, const Weight & r) const
        {
            return l.next_event_time > r.next_event_time;
        }
    };

    std::vector<Weight> weights;
    size_t num_picks = 0;

public:

    WeightedDistributionSimple()
    {
    }

    WeightedDistributionSimple(std::initializer_list<float> il)
    {
        weights.reserve(il.size());
        for (float w : il)
            add_weight(w);
    }

    void add_weight(float w)
    {
        weights.emplace_back(1.0f / w, weights.size());
    }

    size_t num_weights() const
    {
        return weights.size();
    }

    // you need to call this once after adding all the weights to this WeightedDistribution
    // otherwise the first couple of picks will always be deterministic.
    template<typename Random>
    void initialize_randomness(Random & randomness)
    {
        for (Weight & w : weights)
        {
            w.next_event_time = std::uniform_real_distribution<float>(0.0f, w.average_time_between_events)(randomness);
        }
        std::make_heap(weights.begin(), weights.end(), CompareByNextTime());
    }

    // use this to pick a random item. it will give the distribution that you asked for
    // but try to not repeat the same item too often, or to let too much time pass
    // since an item was picked before it gets picked again.
    template<typename Random>
    size_t pick_random(Random & randomness)
    {
        ++num_picks;
        if (num_picks > (weights.size() * 16))
        {
            float to_subtract = weights.front().next_event_time;
            for (Weight & w : weights)
                w.next_event_time -= to_subtract;
            num_picks = 0;
        }

        Weight & picked = weights.front();
        size_t result = picked.original_index;
        picked.next_event_time += std::uniform_real_distribution<float>(0.0f, picked.average_time_between_events)(randomness);
        //picked.next_event_time += std::exponential_distribution<float>(1.0f / picked.average_time_between_events)(randomness);
        //picked.next_event_time += picked.average_time_between_events;
        heap_top_updated(weights.begin(), weights.end(), CompareByNextTime());
        return result;
    }
};

class WeightedDistributionLinear
{
    struct Weight
    {
        Weight(float frequency, size_t original_index)
            : next_event_time(frequency)
            , average_time_between_events(frequency)
            , original_index(original_index)
        {
        }

        float next_event_time;
        float average_time_between_events;
        size_t original_index;
    };
    struct CompareByNextTime
    {
        bool operator()(const Weight & l, const Weight & r) const
        {
            return l.next_event_time < r.next_event_time;
        }
    };

    std::vector<Weight> weights;

public:

    WeightedDistributionLinear()
    {
    }

    WeightedDistributionLinear(std::initializer_list<float> il)
    {
        weights.reserve(il.size());
        for (float w : il)
            add_weight(w);
    }

    void add_weight(float w)
    {
        weights.emplace_back(1.0f / w, weights.size());
    }

    size_t num_weights() const
    {
        return weights.size();
    }

    // you need to call this once after adding all the weights to this WeightedDistribution
    // otherwise the first couple of picks will always be deterministic.
    template<typename Random>
    void initialize_randomness(Random & randomness)
    {
        for (Weight & w : weights)
        {
            w.next_event_time = std::uniform_real_distribution<float>(0.0f, w.average_time_between_events)(randomness);
        }
    }

    // use this to pick a random item. it will give the distribution that you asked for
    // but try to not repeat the same item too often, or to let too much time pass
    // since an item was picked before it gets picked again.
    template<typename Random>
    size_t pick_random(Random & randomness)
    {
        auto chosen = std::min_element(weights.begin(), weights.end(), CompareByNextTime());
        size_t result = chosen->original_index;
        float to_subtract = chosen->next_event_time;
        for (Weight & w : weights)
        {
            w.next_event_time -= to_subtract;
        }
        chosen->next_event_time = std::uniform_real_distribution<float>(0.0f, chosen->average_time_between_events)(randomness);
        return result;
    }
};

class WeightedDistributionFixedPoint
{
    static constexpr float fixed_point_multiplier = 1024.0f * 1024.0f;
    struct Weight
    {
        Weight(float frequency, size_t original_index)
            : average_time_between_events(static_cast<uint32_t>(frequency * fixed_point_multiplier + 0.5f))
            , original_index(original_index)
        {
            next_event_time = average_time_between_events;
        }

        uint32_t next_event_time;
        uint32_t average_time_between_events;
        size_t original_index;
    };
    struct CompareByNextTime
    {
        uint32_t reference_point = 0;
        bool operator()(const Weight & l, const Weight & r) const
        {
            return (l.next_event_time - reference_point) > (r.next_event_time - reference_point);
        }
    };

    std::vector<Weight> weights;

public:

    WeightedDistributionFixedPoint()
    {
    }

    WeightedDistributionFixedPoint(std::initializer_list<float> il)
    {
        weights.reserve(il.size());
        for (float w : il)
            add_weight(w);
    }

    // how these values were chosen:
    // min_weight was chosen so that the largest number we add in pick_random
    // can be std::numeric_limits<uint32_t>::max() / 4. that gives us enough
    // space to not have to worry about things wrapping around.
    //
    // max_weight was chosen so that its distribution in pick_random would be
    // std::uniform_int_distribution<uint32_t>(0, 102). the bigger numbers we
    // allow, the smaller the range on that distribution. and then similar
    // numbers start to behave the same. so for example if we allowed numbers
    // up to 32768 then 32000 behaves exactly the same as 32768. (they'd both
    // have 32 as an upper limit) I chose a round number that still allows
    // us to notice 1% differences. meaning if you subtract 1% from the max
    // you will actually have a 1% lower chance of being picked
    //
    // if you run into either limit, just grow your range in the other
    // direction meaning if you have a lot of large numbers, maybe add some
    // small numbers instead. the only thing that matters is the ratio between
    // the numbers, not the absolute value of the number. so make sure to use
    // the full range. you could also do an automatic normalization step on
    // your inputs to ensure that they use this range.
    //
    // if you really need a bigger range (because one item needs to happen more
    // than ten million times more often than another) consider changing this
    // to use uint64_t instead of uint32_t and double instead of float and then
    // increase the fixed_point_multiplier to 1024.0 * 1024.0 * 1024.0 * 1024.0
    static constexpr float min_weight = 1.0f / 1024.0f;
    static constexpr float max_weight = 10240.0f;

    void add_weight(float w)
    {
        // since I'm using fixed point math, I only support a certain range
        assert(w >= min_weight);
        assert(w <= max_weight);
        weights.emplace_back(1.0f / w, weights.size());
    }

    size_t num_weights() const
    {
        return weights.size();
    }

    // you need to call this once after adding all the weights to this WeightedDistribution
    // otherwise the first couple of picks will always be deterministic.
    template<typename Random>
    void initialize_randomness(Random & randomness)
    {
        for (Weight & w : weights)
        {
            w.next_event_time = std::uniform_int_distribution<uint32_t>(0, w.average_time_between_events)(randomness);
        }
        std::make_heap(weights.begin(), weights.end(), CompareByNextTime{0});
    }

    // use this to pick a random item. it will give the distribution that you asked for
    // but try to not repeat the same item too often, or to let too much time pass
    // since an item was picked before it gets picked again.
    template<typename Random>
    size_t pick_random(Random & randomness)
    {
        Weight & picked = weights.front();
        size_t result = picked.original_index;
        uint32_t reference_point = picked.next_event_time;
        picked.next_event_time += std::uniform_int_distribution<uint32_t>(0, picked.average_time_between_events)(randomness);
        heap_top_updated(weights.begin(), weights.end(), CompareByNextTime{reference_point});
        return result;
    }
};

class WeightedDistributionBlendedFixedPoint
{
    static constexpr float fixed_point_multiplier = 1024.0f * 1024.0f;
    struct Weight
    {
        Weight(float frequency, size_t original_index)
            : average_time_between_events(static_cast<uint32_t>(frequency * fixed_point_multiplier + 0.5f))
            , original_index(original_index)
        {
            next_event_time = average_time_between_events;
        }

        uint32_t next_event_time;
        uint32_t average_time_between_events;
        size_t original_index;
    };
    struct CompareByNextTime
    {
        uint32_t reference_point = 0;
        bool operator()(const Weight & l, const Weight & r) const
        {
            return (l.next_event_time - reference_point) > (r.next_event_time - reference_point);
        }
    };

    std::vector<Weight> weights;

public:

    WeightedDistributionBlendedFixedPoint()
    {
    }

    WeightedDistributionBlendedFixedPoint(std::initializer_list<float> il)
    {
        weights.reserve(il.size());
        for (float w : il)
            add_weight(w);
    }

    // how these values were chosen:
    // min_weight was chosen so that the largest number we add in pick_random
    // can be std::numeric_limits<uint32_t>::max() / 4. that gives us enough
    // space to not have to worry about things wrapping around.
    //
    // max_weight was chosen so that its distribution in pick_random would be
    // std::uniform_int_distribution<uint32_t>(0, 102). the bigger numbers we
    // allow, the smaller the range on that distribution. and then similar
    // numbers start to behave the same. so for example if we allowed numbers
    // up to 32768 then 32000 behaves exactly the same as 32768. (they'd both
    // have 32 as an upper limit) I chose a round number that still allows
    // us to notice 1% differences. meaning if you subtract 1% from the max
    // you will actually have a 1% lower chance of being picked
    //
    // if you run into either limit, just grow your range in the other
    // direction meaning if you have a lot of large numbers, maybe add some
    // small numbers instead. the only thing that matters is the ratio between
    // the numbers, not the absolute value of the number. so make sure to use
    // the full range. you could also do an automatic normalization step on
    // your inputs to ensure that they use this range.
    //
    // if you really need a bigger range (because one item needs to happen more
    // than ten million times more often than another) consider changing this
    // to use uint64_t instead of uint32_t and double instead of float and then
    // increase the fixed_point_multiplier to 1024.0 * 1024.0 * 1024.0 * 1024.0
    static constexpr float min_weight = 1.0f / 1024.0f;
    static constexpr float max_weight = 10240.0f;

    void add_weight(float w)
    {
        // since I'm using fixed point math, I only support a certain range
        assert(w >= min_weight);
        assert(w <= max_weight);
        weights.emplace_back(1.0f / w, weights.size());
    }

    size_t num_weights() const
    {
        return weights.size();
    }

    // you need to call this once after adding all the weights to this WeightedDistribution
    // otherwise the first couple of picks will always be deterministic.
    template<typename Random>
    void initialize_randomness(Random & randomness)
    {
        for (Weight & w : weights)
        {
            w.next_event_time = std::uniform_int_distribution<uint32_t>(0, w.average_time_between_events)(randomness);
        }
        std::make_heap(weights.begin(), weights.end(), CompareByNextTime{0});
    }

    // use this to pick a random item. it will give the distribution that you asked for
    // but try to not repeat the same item too often, or to let too much time pass
    // since an item was picked before it gets picked again.
    template<typename Random>
    size_t pick_random(Random & randomness)
    {
        Weight & picked = weights.front();
        size_t result = picked.original_index;
        uint32_t reference_point = picked.next_event_time;
        uint32_t to_add = std::uniform_int_distribution<uint32_t>(0, picked.average_time_between_events)(randomness);
        to_add *= 3;
        to_add /= 4;
        to_add += picked.average_time_between_events / 4;
        picked.next_event_time += to_add;
        heap_top_updated(weights.begin(), weights.end(), CompareByNextTime{reference_point});
        return result;
    }
};

class WeightedDistributionLinearFixedPoint
{
    static constexpr float fixed_point_multiplier = 1024.0f * 1024.0f;
    struct Weight
    {
        Weight(float frequency, size_t original_index)
            : next_event_time(static_cast<uint32_t>(frequency * fixed_point_multiplier + 0.5f))
            , average_time_between_events(static_cast<uint32_t>(frequency * fixed_point_multiplier + 0.5f))
            , original_index(original_index)
        {
        }

        uint32_t next_event_time;
        uint32_t average_time_between_events;
        size_t original_index;
    };
    struct CompareByNextTime
    {
        bool operator()(const Weight & l, const Weight & r) const
        {
            return l.next_event_time < r.next_event_time;
        }
    };

    std::vector<Weight> weights;

public:

    WeightedDistributionLinearFixedPoint()
    {
    }

    WeightedDistributionLinearFixedPoint(std::initializer_list<float> il)
    {
        weights.reserve(il.size());
        for (float w : il)
            add_weight(w);
    }

    void add_weight(float w)
    {
        weights.emplace_back(1.0f / w, weights.size());
    }

    size_t num_weights() const
    {
        return weights.size();
    }

    // you need to call this once after adding all the weights to this WeightedDistribution
    // otherwise the first couple of picks will always be deterministic.
    template<typename Random>
    void initialize_randomness(Random & randomness)
    {
        for (Weight & w : weights)
        {
            w.next_event_time = std::uniform_int_distribution<uint32_t>(0, w.average_time_between_events)(randomness);
        }
    }

    // use this to pick a random item. it will give the distribution that you asked for
    // but try to not repeat the same item too often, or to let too much time pass
    // since an item was picked before it gets picked again.
    template<typename Random>
    size_t pick_random(Random & randomness)
    {
        auto chosen = std::min_element(weights.begin(), weights.end(), CompareByNextTime());
        size_t result = chosen->original_index;
        uint32_t to_subtract = chosen->next_event_time;
        for (Weight & w : weights)
        {
            w.next_event_time -= to_subtract;
        }
        chosen->next_event_time = std::uniform_int_distribution<uint32_t>(0, chosen->average_time_between_events)(randomness);
        return result;
    }
};

template<typename Randomness>
bool random_success(float chance, float & success_state, float & fail_state, Randomness & randomness)
{
    float success_frequency = 1.0f / chance;
    float fail_frequency = 1.0f / (1.0f - chance);
    if (success_state < fail_state)
    {
        fail_state -= success_state;
        success_state = std::uniform_real_distribution<float>(0.0f, success_frequency)(randomness);
        return true;
    }
    else
    {
        success_state -= fail_state;
        fail_state = std::uniform_real_distribution<float>(0.0f, fail_frequency)(randomness);
        return false;
    }
}

inline int round_positive_float(float f)
{
    return static_cast<int>(f + 0.5f);
}

class ControlledRandom
{
    float state = 1.0f;
    int index = 0;
    static constexpr const float constant_to_multiply[101] =
    {
        1.0f,
        0.999842823f, 0.999372184f, 0.99858737f, 0.997489989f, 0.996079504f, // 5%
        0.994353354f, 0.992320299f, 0.989976823f, 0.987323165f, 0.984358072f, // 10%
        0.98108995f, 0.977510273f, 0.973632514f, 0.969447076f, 0.964966297f, // 15%
        0.960183799f, 0.955135703f, 0.949759007f, 0.94411546f, 0.93817538f, // 20%
        0.931944132f, 0.925439596f, 0.918646991f, 0.91158092f, 0.904245615f, // 25%
        0.896643937f, 0.888772905f, 0.880638301f, 0.872264326f, 0.863632858f, // 30%
        0.854712844f, 0.845594227f, 0.836190343f, 0.826578021f, 0.816753447f, // 35%
        0.806658566f, 0.796402514f, 0.785905063f, 0.775190175f, 0.764275074f, // 40%
        0.753200769f, 0.741862416f, 0.730398834f, 0.71871227f, 0.706894219f, // 45%
        0.694856822f, 0.68264246f, 0.670327544f, 0.657848954f, 0.645235062f, // 50%
        0.6324597f, 0.619563162f, 0.606526911f, 0.593426645f, 0.580169916f, // 55%
        0.566839218f, 0.553292334f, 0.539853752f, 0.526208699f, 0.512536764f, // 60%
        0.498813927f, 0.485045046f, 0.471181333f, 0.457302243f, 0.443413943f, // 65%
        0.429503262f, 0.415506482f, 0.401567012f, 0.38765198f, 0.373695225f, // 70%
        0.359745115f, 0.345868856f, 0.331981093f, 0.31815201f, 0.304365695f, // 75%
        0.290644556f, 0.277024776f, 0.263462812f, 0.249986023f, 0.236542806f, // 80%
        0.223382816f, 0.210130796f, 0.197115764f, 0.184175551f, 0.171426639f, // 85%
        0.158810839f, 0.146292359f, 0.133954003f, 0.121768393f, 0.109754287f, // 90%
        0.0979399607f, 0.0863209665f, 0.0748278722f, 0.0635780841f, 0.0524956733f, // 95%
        0.0415893458f, 0.0308760721f, 0.0203953665f, 0.0100950971f, //99%
        -1.0f,
    };
public:
    explicit ControlledRandom(float odds)
    {
        if (odds <= 0.0f)
            index = 0;
        else if (odds >= 1.0f)
            index = 100;
        else
            index = std::min(std::max(round_positive_float(odds * 100.0f), 1), 99);
    }

    template<typename Randomness>
    bool random_success(Randomness & randomness)
    {
        state *= constant_to_multiply[index];
        if (std::uniform_real_distribution<float>()(randomness) <= state)
            return false;
        state = 1.0f;
        return true;
    }
};

template<typename Randomness>
bool random_success(float odds, float & accumulator, Randomness & randomness)
{
    if (odds <= 0.0f)
        return false;
    if (odds >= 1.0f)
        return true;
    static constexpr const float constant_to_add[99] =
    {
    #if 0
        0.000156023569f, 0.000619351282f, 0.00138709578f, 0.00244869851f, 0.00380068412f, // 5%
        0.00543773733f, 0.00735962857f, 0.00954720005f, 0.0120140798f, 0.0147433262f, // 10%
        0.0177349187f, 0.0209783465f, 0.0244794339f, 0.0282241367f, 0.0322206244f, // 15%
        0.0364497826f, 0.0409160033f, 0.0456289202f, 0.0505514443f, 0.0557133779f, // 20%
        0.0610876232f, 0.066686213f, 0.0724886656f, 0.0785209239f, 0.0847437829f, // 25%
        0.0911894143f, 0.0978297591f, 0.1046637f, 0.111723736f, 0.118931472f, // 30%
        0.12638548f, 0.133978009f, 0.141796023f, 0.149802625f, 0.158013135f, // 35%
        0.16635555f, 0.174957931f, 0.183628172f, 0.192449331f, 0.201513708f, // 40%
        0.210939676f, 0.22040233f, 0.229908854f, 0.239530236f, 0.249333739f, // 45%
        0.259924531f, 0.270470858f, 0.281053722f, 0.291511059f, 0.302089453f, // 50%
        // switch to other algorithm above 50%
        /*0.683660269f, 0.663464427f, 0.643620014f, 0.624245048f, 0.605182767f, // 55%
        0.586630821f, 0.568377256f, 0.550435901f, 0.532860041f, 0.515577078f, // 60%
        0.498669922f, 0.482016385f, 0.465579212f, 0.449533701f, 0.433685422f, // 65%
        0.418219864f, 0.402918875f, 0.387824595f, 0.373050272f, 0.358398497f, // 70%
        0.344003141f, 0.329826474f, 0.315870941f, 0.302169979f, 0.288610339f, // 75%
        0.275245905f, 0.262090266f, 0.249078989f, 0.236272335f, 0.223585606f, // 80%
        0.211093813f, 0.198834866f, 0.186576962f, 0.174564004f, 0.162662327f, // 85%
        0.15095216f, 0.13932249f, 0.127916843f, 0.116557524f, 0.105371997f, // 90%
        0.0943201929f, 0.083389625f, 0.072578311f, 0.0618926659f, 0.0512852818f, // 95%
        0.0408272594f, 0.0304889455f, 0.0202144794f, 0.010063896f, // 99%*/
        0.794855833f, 0.768348455f, 0.742541134f, 0.717434883f, 0.69296813f, // 55%
        0.669051588f, 0.645910501f, 0.623230278f, 0.601179361f, 0.579536617f, // 60%
        0.558575749f, 0.538084924f, 0.518063605f, 0.498465031f, 0.479341745f, // 65%
        0.460635543f, 0.44232446f, 0.424389422f, 0.406874001f, 0.389687985f, // 70%
        0.372932971f, 0.356447816f, 0.340349168f, 0.324524373f, 0.309056222f, // 75%
        0.293781132f, 0.278970391f, 0.264264494f, 0.249956161f, 0.235906407f, // 80%
        0.222084641f, 0.208510607f, 0.195165694f, 0.182083905f, 0.16918914f, // 85%
        0.156559914f, 0.144163489f, 0.131954402f, 0.119962737f, 0.108136661f, // 90%
        0.0965787098f, 0.0851326957f, 0.0739006326f, 0.0628736243f, 0.0519751795f, // 95%
        0.0412430242f, 0.0306835137f, 0.0203149989f, 0.0100731095f, //99%
    #else
        0.999842823f, 0.999372184f, 0.99858737f, 0.997489989f, 0.996079504f, // 5%
        0.994353354f, 0.992320299f, 0.989976823f, 0.987323165f, 0.984358072f, // 10%
        0.98108995f, 0.977510273f, 0.973632514f, 0.969447076f, 0.964966297f, // 15%
        0.960183799f, 0.955135703f, 0.949759007f, 0.94411546f, 0.93817538f, // 20%
        0.931944132f, 0.925439596f, 0.918646991f, 0.91158092f, 0.904245615f, // 25%
        0.896643937f, 0.888772905f, 0.880638301f, 0.872264326f, 0.863632858f, // 30%
        0.854712844f, 0.845594227f, 0.836190343f, 0.826578021f, 0.816753447f, // 35%
        0.806658566f, 0.796402514f, 0.785905063f, 0.775190175f, 0.764275074f, // 40%
        0.753200769f, 0.741862416f, 0.730398834f, 0.71871227f, 0.706894219f, // 45%
        0.694856822f, 0.68264246f, 0.670327544f, 0.657848954f, 0.645235062f, // 50%
        0.6324597f, 0.619563162f, 0.606526911f, 0.593426645f, 0.580169916f, // 55%
        0.566839218f, 0.553292334f, 0.539853752f, 0.526208699f, 0.512536764f, // 60%
        0.498813927f, 0.485045046f, 0.471181333f, 0.457302243f, 0.443413943f, // 65%
        0.429503262f, 0.415506482f, 0.401567012f, 0.38765198f, 0.373695225f, // 70%
        0.359745115f, 0.345868856f, 0.331981093f, 0.31815201f, 0.304365695f, // 75%
        0.290644556f, 0.277024776f, 0.263462812f, 0.249986023f, 0.236542806f, // 80%
        0.223382816f, 0.210130796f, 0.197115764f, 0.184175551f, 0.171426639f, // 85%
        0.158810839f, 0.146292359f, 0.133954003f, 0.121768393f, 0.109754287f, // 90%
        0.0979399607f, 0.0863209665f, 0.0748278722f, 0.0635780841f, 0.0524956733f, // 95%
        0.0415893458f, 0.0308760721f, 0.0203953665f, 0.0100950971f, //99%
    #endif
    };
    int index = std::min(std::max(round_positive_float(odds * 100.0f) - 1, 0), 98);

#if 0
    if (index >= 50)
    {
        if (accumulator == 0.0f)
            accumulator = constant_to_add[index];
        if (std::uniform_real_distribution<float>()(randomness) > accumulator)
        {
            accumulator = constant_to_add[index];
            return true;
        }
        else
        {
            accumulator *= 0.25f;
            return false;
        }
    }
    else
    {
        accumulator += constant_to_add[index];
        if (std::uniform_real_distribution<float>()(randomness) < accumulator)
        {
            accumulator = 0.0f;
            return true;
        }
        else
            return false;
    }
#else
    if (accumulator == 0.0f)
        accumulator = 1.0f;
    accumulator *= constant_to_add[index];
    if (std::uniform_real_distribution<float>()(randomness) > accumulator)
    {
        accumulator = 1.0f;
        return true;
    }
    else
        return false;
#endif
}
