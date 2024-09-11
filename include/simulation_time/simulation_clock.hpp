#pragma once

#include <cassert>
#include <chrono>
#include <stdexcept>
#include <utility>

namespace ngen {

//! Simulation Clock
//!
//! Represents simulation ticks monotonically increasing from zero, and
//! provides conversion utilities between simulation ticks and
//! calendar time.
//!
//! Implements named requirements: Clock (https://en.cppreference.com/w/cpp/named_req/Clock)
//!
struct simulation_clock final
{
    /* Clock named requirements */

    using rep = std::int64_t; // TODO: double vs std::int64_t
    using period = std::ratio<1>;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<simulation_clock, duration>;
    static constexpr bool is_steady = false;

    //! Current Simulation Time
    //! \returns The current point in time the simulation is at.
    //! \post Return value must have a non-negative representation value..
    static time_point now() noexcept;

    /* simulation_clock-specific types/interface */

    // explicit simulation time type aliases //
    using simulation_rep = rep;
    using simulation_period = period;
    using simulation_duration = duration;
    using simulation_time_point = time_point;

    // explicit calendar time type aliases //
    using calendar_clock = std::chrono::system_clock;
    using calendar_rep = calendar_clock::rep;
    using calendar_period = calendar_clock::period;
    using calendar_duration = calendar_clock::duration;
    using calendar_time_point = calendar_clock::time_point;

    // simulation-calendar conversion functions //

    //! Convert a calendar time point to a simulation time point.
    //! \param calendar_time Calendar time point.
    //! \returns The corresponding simulation time point.
    //! \pre simulation_clock::set_epoch must have been called prior to calling this.
    static simulation_time_point from_calendar(const calendar_time_point& calendar_time);

    //! Convert a simulation time point to a calendar time point.
    //! \param sim_time Simulation time point.
    //! \returns The corresponding calendar time point.
    //! \pre simulation_clock::set_epoch must have been called prior to calling this.
    static calendar_time_point to_calendar(const simulation_time_point& sim_time);

    //! Convert a std::time_t value to a simulation time point.
    //! \see simulation_clock::from_calendar
    //! \param time Unix epoch-based time value
    //! \returns The corresponding simulation time point.
    //! \pre simulation_clock::set_epoch must have been called prior to calling this.
    static simulation_time_point from_time_t(std::time_t t);

    //! Convert a simulation time point to a std::time_t value.
    //! \see simulation_clock::to_calendar
    //! \param sim_time Simulation time point.
    //! \returns The corresponding std::time_t value.
    //! \pre simulation_clock::set_epoch must have been called prior to calling this.
    static std::time_t to_time_t(const simulation_time_point& sim_time);

    // simulation management functions //

    //! Unitialized epoch value, aka the epoch of typename simulation_clock::calendar_clock.
    static constexpr calendar_time_point uninitialized_epoch() noexcept;

    //! Get the simulation epoch.
    //! \see simulation_clock::calendar_time_point
    //! \see simulation_clock::set_epoch
    //! \returns
    //! Calendar time point representing the simulation start date/time.
    //! If the epoch has not been set, then simulation_clock::uninitialized_epoch
    //! is returned.
    static calendar_time_point epoch() noexcept;

    //! Set the simulation epoch.
    //! \note This function must only be called once.
    //! \param calendar_time Calendar time set as the epoch.
    //! \throws std::logic_error Thrown when this function is called more than once.
    static void set_epoch(calendar_time_point calendar_time);

    //! Check if the simulation epoch has been set.
    //! \returns true when the epoch is set, and false otherwise.
    static bool has_epoch() noexcept;

    //! Forward the simulation by N ticks.
    //! \param steps Number of steps to tick.
    //! \note Ticking does *not* require that simulation_clock::set_epoch has been called.
    //! \see simulation_clock::period
    //! \pre `steps` must be non-negative.
    //! \throws std::domain_error Throws when the precondition is violated.
    static void tick(rep steps = 1);

    //! Forward the simulation by some duration.
    //! \param duration Duration to move simulation forward.
    //! \note Ticking does *not* require that simulation_clock::set_epoch has been called.
    //! \pre `duration` must be non-negative.
    //! \throws std::domain_error Throws when the precondition is violated.
    template<typename Rep, typename Period>
    static void tick(const std::chrono::duration<Rep, Period>& duration);

  private:
    //! Simulation epoch, the calendar start time of the simulation.
    //! \returns Mutable reference to the internal calendar epoch.
    static calendar_time_point& internal_epoch();

    //! Current simulation tick.
    //! \returns Mutable reference to the current internal simulation tick.
    static simulation_time_point& internal_now();
};

/* simulation_clock Implementation */

inline constexpr simulation_clock::calendar_time_point simulation_clock::uninitialized_epoch() noexcept
{
    return {};
}

inline simulation_clock::calendar_time_point& simulation_clock::internal_epoch()
{
    static calendar_time_point epoch = simulation_clock::uninitialized_epoch();
    return epoch;
}

inline simulation_clock::time_point& simulation_clock::internal_now()
{
    static simulation_time_point now{}; // Default initialize to zero value
    return now;
}

inline simulation_clock::time_point simulation_clock::now() noexcept
{
    return simulation_clock::internal_now();
}

inline simulation_clock::time_point simulation_clock::from_calendar(const calendar_time_point& calendar_time)
{
    assert(simulation_clock::has_epoch());

    return simulation_time_point{
        std::chrono::duration_cast<simulation_duration>(simulation_clock::internal_epoch() - calendar_time)
    };
}

inline simulation_clock::calendar_time_point simulation_clock::to_calendar(const simulation_time_point& sim_time)
{
    assert(simulation_clock::has_epoch());

    simulation_duration sim_duration = sim_time.time_since_epoch();
    return simulation_clock::internal_epoch() + std::chrono::duration_cast<calendar_duration>(sim_duration);
}

inline simulation_clock::time_point simulation_clock::from_time_t(std::time_t t)
{
    assert(simulation_clock::has_epoch());

    return simulation_clock::from_calendar(calendar_clock::from_time_t(t));
}

inline std::time_t simulation_clock::to_time_t(const simulation_time_point& sim_time)
{
    assert(simulation_clock::has_epoch());

    return calendar_clock::to_time_t(simulation_clock::to_calendar(sim_time));
}

inline simulation_clock::calendar_time_point simulation_clock::epoch() noexcept
{
    return simulation_clock::internal_epoch();
}

inline void simulation_clock::set_epoch(calendar_time_point calendar_time)
{
    if (simulation_clock::has_epoch()) {
        throw std::logic_error{"Attempting to set simulation_clock epoch after already being set."};
    }

    simulation_clock::internal_epoch() = std::move(calendar_time);
}

inline bool simulation_clock::has_epoch() noexcept
{
    return simulation_clock::internal_epoch() != simulation_clock::uninitialized_epoch();
}

inline void simulation_clock::tick(rep steps)
{
    if (steps < 0) {
        throw std::domain_error{"Given `steps` is negative when it must be non-negative."};
    }

    simulation_clock::internal_now() += simulation_clock::duration{steps};
}

template<typename Rep, typename Period>
inline void simulation_clock::tick(const std::chrono::duration<Rep, Period>& duration)
{
    if (duration < std::chrono::duration<Rep, Period>::zero()) {
        throw std::domain_error{"Given `duration` is negative when it must be non-negative."};
    }

    simulation_clock::internal_now() += std::chrono::duration_cast<simulation_clock::duration>(duration);
}

} // namespace ngen
