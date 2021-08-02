#include "catch.hpp"

#include <sstream>
#include <cstdio>
#include <iomanip>

#include "arc_feed_util.hh"
using namespace interp_shared;
using Catch::Matchers::WithinAbs;

#if 0
SCENARIO("Arc calculations")
{
    int turns = GENERATE(-1,1);
    const double cx = 1.5;
    const double cy = 3.5;

    for (double theta1 = -M_PI; theta1 <= M_PI; theta1 += M_PI/5.) {
        printf("\ntheta1 %f\n\n", theta1);
        for (double theta2 = -M_PI; theta2 <= M_PI; theta2 += M_PI/5.) {
            auto data = CircleAngle(
                cos(theta1)+cx,
                sin(theta1)+cy,
                cx,
                cy,
                turns,
                cos(theta2)+cx,
                sin(theta2)+cy,
                true);
            printf("Got principle angle %f, full angle %f\n", data.principal_angle, data.full_angle());
            //REQUIRE(data.principal_angle * (double)turns >= 0);
        }
    }
}
#endif

SCENARIO("Arc Feed with different principal angles crossing the +pi / -pi range of atan2")
{
    const double cx = 1.5;
    const double cy = 3.5;
    int turn = GENERATE(-1,1);

    GIVEN("arc endpoints far apart in negative direction") {
        const double angle_delta = GENERATE(CIRCLE_FUZZ * 1.1, CIRCLE_FUZZ * 10, M_PI_4, M_PI_2, M_PI - CIRCLE_FUZZ);
        const double theta1 = (M_PI - angle_delta / 2.)*turn;
        const double theta2 = -theta1;
        auto data = CircleAngle(
            cos(theta1)+cx,
            sin(theta1)+cy,
            cx,
            cy,
            turn,
            cos(theta2)+cx,
            sin(theta2)+cy,
            true);

        INFO("angle_delta = " << angle_delta << ", theta1 = " << theta1 << "theta2 = " << theta2);
        const double exp_principal_angle = turn*angle_delta;
        const double exp_full_angle = exp_principal_angle;
        CHECK(sgn(data.principal_angle) == turn);
        CHECK_THAT(data.principal_angle, WithinAbs(exp_principal_angle, 1e-8));
        CHECK_THAT(data.full_angle(), WithinAbs(exp_full_angle, 1e-8));
    }
    GIVEN("arc endpoints close together in negative direction within fuzz and allowing an implicit full turn") {
        const double angle_delta = GENERATE(CIRCLE_FUZZ *.99, 1e-9);
        const double theta1 = (M_PI - angle_delta / 2.)*turn;
        const double theta2 = -theta1;
        auto data = CircleAngle(
            cos(theta1)+cx,
            sin(theta1)+cy,
            cx,
            cy,
            turn,
            cos(theta2)+cx,
            sin(theta2)+cy,
            true);

        // Triggers the implicit full turn,
        const double exp_principal_angle = angle_delta * turn;
        const double exp_full_angle = (2.*M_PI+angle_delta)*turn;
        CHECK(sgn(data.principal_angle) == turn);
        CHECK_THAT(data.principal_angle, WithinAbs(exp_principal_angle, 1e-8));
        CHECK_THAT(data.full_angle(), WithinAbs(exp_full_angle, 1e-8));
    }
    GIVEN("arc endpoints close together in negative direction within fuzz with no implicit full turn") {
        const double angle_delta = GENERATE(CIRCLE_FUZZ *.99, 1e-9);
        const double theta1 = (M_PI - angle_delta / 2.)*turn;
        const double theta2 = -theta1;
        auto data = CircleAngle(
            cos(theta1)+cx,
            sin(theta1)+cy,
            cx,
            cy,
            turn,
            cos(theta2)+cx,
            sin(theta2)+cy,
            false);

        // Triggers the implicit full turn,
        INFO("angle_delta = " << angle_delta << ", theta1 = " << theta1 << "theta2 = " << theta2);
        const double exp_principal_angle = angle_delta*turn;
        const double exp_full_angle = exp_principal_angle;
        CHECK(sgn(data.principal_angle) == turn);
        CHECK_THAT(data.principal_angle, WithinAbs(exp_principal_angle, 1e-8));
        CHECK_THAT(data.full_angle(), WithinAbs(exp_full_angle, 1e-8));
    }
    GIVEN("arc endpoints close together in positive direction") {
        const double angle_delta = GENERATE(CIRCLE_FUZZ / 4, CIRCLE_FUZZ, M_PI_4, M_PI_2, M_PI-CIRCLE_FUZZ);
        const double theta1 = -(M_PI - angle_delta / 2.)*turn;
        const double theta2 = -theta1;
        auto data = CircleAngle(
            cos(theta1)+cx,
            sin(theta1)+cy,
            cx,
            cy,
            turn,
            cos(theta2)+cx,
            sin(theta2)+cy,
            true);

        INFO("angle_delta = " << angle_delta << ", theta1 = " << theta1 << "theta2 = " << theta2);
        // Wrap around slightly less than one full turn because the displacement is slightly
        const double exp_principal_angle = (2.*M_PI-angle_delta)*turn;
        const double exp_full_angle = exp_principal_angle;
        CHECK(sgn(data.principal_angle) == turn);
        CHECK_THAT(data.principal_angle, WithinAbs(exp_principal_angle, 1e-8));
        CHECK_THAT(data.full_angle(), WithinAbs(exp_full_angle, 1e-8));
    }
    GIVEN("arc endpoints at 0 and pi") {
        const double angle_delta = GENERATE(M_PI, -M_PI)*turn;
        const double theta1 = 0;
        const double theta2 = angle_delta;
        auto data = CircleAngle(
            cos(theta1)+cx,
            sin(theta1)+cy,
            cx,
            cy,
            turn,
            cos(theta2)+cx,
            sin(theta2)+cy,
            true);

        INFO(std::setprecision(std::numeric_limits<long double>::digits10) << "turns = " << turn << " angle_delta = " << angle_delta << ", theta1 = " << theta1 << " theta2 = " << theta2 << " principle_angle = " << data.principal_angle);
        // NOTE: the principal angle should be in the same direction as the "turns" word (with magnitude 0-2*pi)
        const double exp_principal_angle = fabs(angle_delta)*turn;
        const double exp_full_angle = exp_principal_angle;
        CHECK(sgn(data.principal_angle) == turn);
        CHECK_THAT(data.principal_angle, WithinAbs(exp_principal_angle, 1e-8));
        CHECK_THAT(data.full_angle(), WithinAbs(exp_full_angle, 1e-8));
    }
    GIVEN("arc endpoints near zero and near pi") {
        const double theta1 = GENERATE(CIRCLE_FUZZ, -CIRCLE_FUZZ);
        const double theta2 = M_PI;
        const double angle_delta = turn < 0 ? theta2 - theta1 - 2*M_PI : theta2 - theta1;
        auto data = CircleAngle(
            cos(theta1)+cx,
            sin(theta1)+cy,
            cx,
            cy,
            turn,
            cos(theta2)+cx,
            sin(theta2)+cy,
            true);

        INFO("turns = " << turn << " angle_delta = " << angle_delta << ", theta1 = " << theta1 << " theta2 = " << theta2 << " principle_angle = " << data.principal_angle);
        // NOTE: the principal angle should be in the same direction as the "turns" word (with magnitude 0-2*pi)
        const double exp_principal_angle = angle_delta;
        const double exp_full_angle = exp_principal_angle;
        CHECK(sgn(data.principal_angle) == turn);
        CHECK_THAT(data.principal_angle, WithinAbs(exp_principal_angle, 1e-8));
        CHECK_THAT(data.full_angle(), WithinAbs(exp_full_angle, 1e-8));
    }
    GIVEN("Small positive arc angle") {
        const double theta1 = M_PI/4;
        const double theta2 = theta1 + 0.001;
        const double angle_delta = 0.001;
        auto data = CircleAngle(
            cos(theta1)+cx,
            sin(theta1)+cy,
            cx,
            cy,
            turn,
            cos(theta2)+cx,
            sin(theta2)+cy,
            true);

        INFO("turns = " << turn << " angle_delta = " << angle_delta << ", theta1 = " << theta1 << " theta2 = " << theta2 << " principle_angle = " << data.principal_angle);
        // NOTE: the principal angle should be in the same direction as the "turns" word (with magnitude 0-2*pi)
        const double exp_principal_angle = turn < 0 ? -2.*M_PI + angle_delta : angle_delta;
        const double exp_full_angle = exp_principal_angle;
        CHECK(sgn(data.principal_angle) == turn);
        CHECK_THAT(data.principal_angle, WithinAbs(exp_principal_angle, 1e-8));
        CHECK_THAT(data.full_angle(), WithinAbs(exp_full_angle, 1e-8));
    }
    GIVEN("Small negative arc angle") {
        const double angle_delta = -0.001;
        const double theta1 = -2/3*M_PI;
        const double theta2 = theta1 + angle_delta;
        auto data = CircleAngle(
            cos(theta1)+cx,
            sin(theta1)+cy,
            cx,
            cy,
            turn,
            cos(theta2)+cx,
            sin(theta2)+cy,
            true);

        INFO("turns = " << turn << " angle_delta = " << angle_delta << ", theta1 = " << theta1 << " theta2 = " << theta2 << " principle_angle = " << data.principal_angle);
        // NOTE: the principal angle should be in the same direction as the "turns" word (with magnitude 0-2*pi)
        const double exp_principal_angle = turn < 0 ? angle_delta : 2.*M_PI + angle_delta;
        const double exp_full_angle = exp_principal_angle;
        CHECK(sgn(data.principal_angle) == turn);
        CHECK_THAT(data.principal_angle, WithinAbs(exp_principal_angle, 1e-8));
        CHECK_THAT(data.full_angle(), WithinAbs(exp_full_angle, 1e-8));
    }
}
