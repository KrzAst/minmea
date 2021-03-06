/*
 * Copyright © 2014 Kosma Moczek <kosma@cloudyourcar.com>
 * This work is free. You can redistribute it and/or modify it under the
 * terms of the Do What The Fuck You Want To Public License, Version 2,
 * as published by Sam Hocevar. See the COPYING file for more details.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <check.h>

#include "minmea.h"

static const char *valid_sequences[] = {
    "$GPTXT,xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
    "$GPTXT,01,01,02,ANTSTATUS=INIT*25",
    "$GPRMC,,V,,,,,,,,,,N*53",
    "$GPVTG,,,,,,,,,N*30",
    "$GPGGA,,,,,,0,00,99.99,,,,,,*48",
    "$GPGSA,A,1,,,,,,,,,,,,,99.99,99.99,99.99*30",
    "$GPGLL,,,,,,V,N*64",
    NULL,
};

static const char *invalid_sequences[] = {
    "$GPTXT,xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
        "xxxxxxxxxxxxxxxxxxxxxxxxxxx",
    "$GPTXT,01,01,02,ANTSTATUS=INIT*26",
    "$GPRMC,,V,,,,,,,,,,N*532",
    "$GPVTG,,,,\xff,,,,,N*30",
    "$$GPGGA,,,,,,0,00,99.99,,,,,,*48",
    "GPGSA,A,1,,,,,,,,,,,,,99.99,99.99,99.99*30",
    "gps: $GPGLL,,,,,,V,N",
    NULL,
};

START_TEST(test_minmea_check)
{
    for (const char **sequence=valid_sequences; *sequence; sequence++)
        ck_assert_msg(minmea_check(*sequence) == true, *sequence);

    for (const char **sequence=invalid_sequences; *sequence; sequence++)
        ck_assert_msg(minmea_check(*sequence) == false, *sequence);
}
END_TEST

START_TEST(test_minmea_scan_c)
{
    char ch;

    ck_assert(minmea_scan("A,123.45", "c", &ch) == true);
    ck_assert_int_eq(ch, 'A');

    ck_assert(minmea_scan("WUT,123.45", "c", &ch) == true);
    ck_assert_int_eq(ch, 'W');

    ck_assert(minmea_scan(",123.45", "c", &ch) == true);
    ck_assert_int_eq(ch, '\0');
}
END_TEST

START_TEST(test_minmea_scan_d)
{
    int direction;

    ck_assert(minmea_scan("K", "d", &direction) == false);

    ck_assert(minmea_scan("", "d", &direction) == true);
    ck_assert(minmea_scan(",foo", "d", &direction) == true);
    ck_assert_int_eq(direction, 0);
    ck_assert(minmea_scan("N", "d", &direction) == true);
    ck_assert_int_eq(direction, 1);
    ck_assert(minmea_scan("S,foo", "d", &direction) == true);
    ck_assert_int_eq(direction, -1);
    ck_assert(minmea_scan("W", "d", &direction) == true);
    ck_assert_int_eq(direction, -1);
    ck_assert(minmea_scan("E,foo", "d", &direction) == true);
    ck_assert_int_eq(direction, 1);
}
END_TEST

START_TEST(test_minmea_scan_f)
{
    int value, scale;

    ck_assert(minmea_scan("-", "f", &value, &scale) == false);
    ck_assert(minmea_scan("10-", "f", &value, &scale) == false);
    ck_assert(minmea_scan("+-10", "f", &value, &scale) == false);
    ck_assert(minmea_scan("12..45", "f", &value, &scale) == false);
    ck_assert(minmea_scan("blah", "f", &value, &scale) == false);
    ck_assert(minmea_scan("12.3.4", "f", &value, &scale) == false);

    ck_assert(minmea_scan(",", "f", &value, &scale) == true);
    ck_assert_int_eq(scale, 0);
    ck_assert(minmea_scan("", "f", &value, &scale) == true);
    ck_assert_int_eq(scale, 0);

    ck_assert(minmea_scan("15.345", "f", &value, &scale) == true);
    ck_assert_int_eq(value, 15345);
    ck_assert_int_eq(scale, 1000);

    ck_assert(minmea_scan("-1.23,V", "f", &value, &scale) == true);
    ck_assert_int_eq(value, -123);
    ck_assert_int_eq(scale, 100);
}
END_TEST

START_TEST(test_minmea_scan_s)
{
    char value[MINMEA_MAX_LENGTH];

    ck_assert(minmea_scan("foo,bar,baz", "s", value) == true);
    ck_assert_str_eq(value, "foo");
    ck_assert(minmea_scan(",bar,baz", "s", value) == true);
    ck_assert_str_eq(value, "");
}
END_TEST

START_TEST(test_minmea_scan_t)
{
    char buf[7];
    buf[sizeof(buf)-1] = 0x42;

    ck_assert(minmea_scan("$GPRM,foo,bar,baz", "t", buf) == false);
    ck_assert(minmea_scan("GPRMC,foo,bar,baz", "t", buf) == false);

    ck_assert(minmea_scan("$GPRMC,foo,bar,baz", "t", buf) == true);
    ck_assert_str_eq(buf, "GPRMC");

    ck_assert(buf[sizeof(buf)-1] == 0x42);
}
END_TEST

START_TEST(test_minmea_scan_D)
{
    struct minmea_date date;

    ck_assert(minmea_scan("$GPXXX,311299", "_D", &date) == true);
    ck_assert_int_eq(date.day, 31);
    ck_assert_int_eq(date.month, 12);
    ck_assert_int_eq(date.year, 99);

    ck_assert(minmea_scan("$GPXXX,,,,,,,,,nope", "_D", &date) == true);
    ck_assert_int_eq(date.day, -1);
    ck_assert_int_eq(date.month, -1);
    ck_assert_int_eq(date.year, -1);
}
END_TEST

START_TEST(test_minmea_scan_T)
{
    struct minmea_time time;

    ck_assert(minmea_scan("$GPXXX,235960", "_T", &time) == true);
    ck_assert_int_eq(time.hours, 23);
    ck_assert_int_eq(time.minutes, 59);
    ck_assert_int_eq(time.seconds, 60);
    ck_assert_int_eq(time.microseconds, 0);

    ck_assert(minmea_scan("$GPXXX,213700.001", "_T", &time) == true);
    ck_assert_int_eq(time.hours, 21);
    ck_assert_int_eq(time.minutes, 37);
    ck_assert_int_eq(time.seconds, 0);
    ck_assert_int_eq(time.microseconds, 1000);

    ck_assert(minmea_scan("$GPXXX,,,,,,,nope", "_T", &time) == true);
    ck_assert_int_eq(time.hours, -1);
    ck_assert_int_eq(time.minutes, -1);
    ck_assert_int_eq(time.seconds, -1);
    ck_assert_int_eq(time.microseconds, -1);
}
END_TEST

START_TEST(test_minmea_scan_complex1)
{
    const char *sentence = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";
    char type[6];
    struct minmea_time time;
    int latitude, latitude_scale, latitude_direction;
    int longitude, longitude_scale, longitude_direction;
    int fix_quality;
    int satellites;
    int hdop, hdop_scale;
    int altitude, altitude_scale; char altitude_units;
    int height, height_scale; char height_units;
    ck_assert(minmea_scan(sentence, "tTfdfdiiffcfc__",
        type,
        &time,
        &latitude, &latitude_scale, &latitude_direction,
        &longitude, &longitude_scale, &longitude_direction,
        &fix_quality,
        &satellites,
        &hdop, &hdop_scale,
        &altitude, &altitude_scale, &altitude_units,
        &height, &height_scale, &height_units) == true);
    ck_assert_str_eq(type, "GPGGA");
    ck_assert_int_eq(time.hours, 12);
    ck_assert_int_eq(time.minutes, 35);
    ck_assert_int_eq(time.seconds, 19);
    ck_assert_int_eq(latitude, 4807038);
    ck_assert_int_eq(latitude_scale, 1000);
    ck_assert_int_eq(latitude_direction, 1);
    ck_assert_int_eq(longitude, 1131000);
    ck_assert_int_eq(longitude_scale, 1000);
    ck_assert_int_eq(longitude_direction, 1);
    ck_assert_int_eq(fix_quality, 1);
    ck_assert_int_eq(satellites, 8);
    ck_assert_int_eq(hdop, 9);
    ck_assert_int_eq(hdop_scale, 10);
    ck_assert_int_eq(altitude, 5454);
    ck_assert_int_eq(altitude_scale, 10);
    ck_assert_int_eq(altitude_units, 'M');
    ck_assert_int_eq(height, 469);
    ck_assert_int_eq(height_scale, 10);
    ck_assert_int_eq(height_units, 'M');

}
END_TEST

START_TEST(test_minmea_scan_complex2)
{
    const char *sentence = "$GPBWC,081837,,,,,,T,,M,,N,*13";
    char type[6];
    struct minmea_time time;
    int latitude, latitude_scale, latitude_direction;
    int longitude, longitude_scale, longitude_direction;
    int bearing_true, bearing_true_scale; char bearing_true_mark;
    int bearing_magnetic, bearing_magnetic_scale; char bearing_magnetic_mark;
    int distance, distance_scale; char distance_units;
    char name[MINMEA_MAX_LENGTH];
    ck_assert(minmea_scan(sentence, "tTfdfdfcfcfcs",
        type,
        &time,
        &latitude, &latitude_scale, &latitude_direction,
        &longitude, &longitude_scale, &longitude_direction,
        &bearing_true, &bearing_true_scale, &bearing_true_mark,
        &bearing_magnetic, &bearing_magnetic_scale, &bearing_magnetic_mark,
        &distance, &distance_scale, &distance_units,
        name) == true);
    ck_assert_str_eq(type, "GPBWC");
    ck_assert_int_eq(time.hours, 8);
    ck_assert_int_eq(time.minutes, 18);
    ck_assert_int_eq(time.seconds, 37);
    ck_assert_int_eq(latitude_scale, 0);
    ck_assert_int_eq(latitude_direction, 0);
    ck_assert_int_eq(longitude_scale, 0);
    ck_assert_int_eq(longitude_direction, 0);
    ck_assert_int_eq(bearing_true_scale, 0);
    ck_assert_int_eq(bearing_true_mark, 'T');
    ck_assert_int_eq(bearing_magnetic_scale, 0);
    ck_assert_int_eq(bearing_magnetic_mark, 'M');
    ck_assert_int_eq(distance_scale, 0);
    ck_assert_int_eq(distance_units, 'N');
    ck_assert_str_eq(name, "");
}
END_TEST

START_TEST(test_minmea_parse_gprmc1)
{
    const char *sentence = "$GPRMC,081836.75,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E";
    struct minmea_gprmc frame = {};
    struct minmea_gprmc expected = {
        .time = { 8, 18, 36, 750000 },
        .valid = true,
        .latitude = -375165,
        .latitude_scale = 100,
        .longitude = 1450736,
        .longitude_scale = 100,
        .speed = 0,
        .speed_scale = 10,
        .course = 3600,
        .course_scale = 10,
        .date = { 13, 9, 98 },
        .variation = 113,
        .variation_scale = 10,
    };
    ck_assert(minmea_check(sentence) == true);
    ck_assert(minmea_parse_gprmc(&frame, sentence) == true);
    ck_assert(!memcmp(&frame, &expected, sizeof(frame)));
}
END_TEST

START_TEST(test_minmea_parse_gprmc2)
{
    const char *sentence = "$GPRMC,,A,3751.65,N,14507.36,W,,,,,";
    struct minmea_gprmc frame = {};
    struct minmea_gprmc expected = {
        .time = { -1, -1, -1, -1 },
        .valid = true,
        .latitude = 375165,
        .latitude_scale = 100,
        .longitude = -1450736,
        .longitude_scale = 100,
        .date = { -1, -1, -1 },
    };
    ck_assert(minmea_check(sentence) == true);
    ck_assert(minmea_parse_gprmc(&frame, sentence) == true);
    ck_assert(!memcmp(&frame, &expected, sizeof(frame)));
}
END_TEST

START_TEST(test_minmea_parse_gpgga1)
{
    const char *sentence = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";
    struct minmea_gpgga frame = {};
    struct minmea_gpgga expected = {
        .time = { 12, 35, 19, 0 },
        .latitude = 4807038,
        .latitude_scale = 1000,
        .longitude = 1131000,
        .longitude_scale = 1000,
        .fix_quality = 1,
        .satellites_tracked = 8,
        .hdop = 9,
        .hdop_scale = 10,
        .altitude = 5454,
        .altitude_scale = 10,
        .altitude_units = 'M',
        .height = 469,
        .height_scale = 10,
        .height_units = 'M',
        .dgps_age = 0,
    };
    ck_assert(minmea_check(sentence) == true);
    ck_assert(minmea_parse_gpgga(&frame, sentence) == true);
    ck_assert(!memcmp(&frame, &expected, sizeof(frame)));
}
END_TEST

START_TEST(test_minmea_usage1)
{
    const char *sentences[] = {
        "$GPRMC,081836,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E*62",
        "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47",
        NULL,
    };

    for (const char **sentence=sentences; *sentence; sentence++) {
        switch (minmea_type(*sentence)) {
            case MINMEA_GPRMC: {
                struct minmea_gprmc frame;
                ck_assert(minmea_parse_gprmc(&frame, *sentence) == true);
            } break;
            
            case MINMEA_GPGGA: {
                struct minmea_gpgga frame;
                ck_assert(minmea_parse_gpgga(&frame, *sentence) == true);
            } break;
            
            default: {
            } break;
        }
    }
}
END_TEST

START_TEST(test_minmea_gettimeofday)
{
    struct minmea_date date = { 14, 2, 14 };
    struct minmea_time time = { 13, 0, 9, 123456 };
    struct timeval tv;
    ck_assert(minmea_gettimeofday(&tv, &date, &time) == 0);
    ck_assert_int_eq(tv.tv_sec, 1392382809);
    ck_assert_int_eq(tv.tv_usec, 123456);

    date.year = -1;
    ck_assert(minmea_gettimeofday(&tv, &date, &time) != 0);
    date.year = 2014;

    time.hours = -1;
    ck_assert(minmea_gettimeofday(&tv, &date, &time) != 0);
}
END_TEST

START_TEST(test_minmea_rescale)
{
    ck_assert(minmea_rescale(42, 0, 3) == 0);
    ck_assert(minmea_rescale(1234, 10, 1) == 123);
    ck_assert(minmea_rescale(1235, 10, 1) == 124);
    ck_assert(minmea_rescale(1234, 10, 1000) == 123400);
}
END_TEST

START_TEST(test_minmea_float)
{
    ck_assert(isnan(minmea_float(42, 0)));
    ck_assert(minmea_float(7, 1) == 7.0);
    ck_assert(minmea_float(-200, 100) == -2.0);
    ck_assert(minmea_float(15, 10) == 1.5);
}
END_TEST

START_TEST(test_minmea_coord)
{
    ck_assert(isnan(minmea_coord(42, 0)));
    ck_assert(minmea_coord(4200, 1) == 42.0);
    ck_assert(minmea_coord(420000, 100) == 42.0);
    ck_assert(minmea_coord(423000, 100) == 42.5);
}
END_TEST

Suite *minmea_suite(void)
{
    Suite *s = suite_create ("minmea");
  
    TCase *tc_check = tcase_create("minmea_check");
    tcase_add_test(tc_check, test_minmea_check);
    suite_add_tcase(s, tc_check);

    TCase *tc_scan = tcase_create("minmea_scan");
    tcase_add_test(tc_scan, test_minmea_scan_c);
    tcase_add_test(tc_scan, test_minmea_scan_d);
    tcase_add_test(tc_scan, test_minmea_scan_f);
    tcase_add_test(tc_scan, test_minmea_scan_s);
    tcase_add_test(tc_scan, test_minmea_scan_t);
    tcase_add_test(tc_scan, test_minmea_scan_D);
    tcase_add_test(tc_scan, test_minmea_scan_T);
    tcase_add_test(tc_scan, test_minmea_scan_complex1);
    tcase_add_test(tc_scan, test_minmea_scan_complex2);
    suite_add_tcase(s, tc_scan);
  
    TCase *tc_parse = tcase_create("minmea_parse");
    tcase_add_test(tc_parse, test_minmea_parse_gprmc1);
    tcase_add_test(tc_parse, test_minmea_parse_gprmc2);
    tcase_add_test(tc_parse, test_minmea_parse_gpgga1);
    suite_add_tcase(s, tc_parse);

    TCase *tc_usage = tcase_create("minmea_usage");
    tcase_add_test(tc_usage, test_minmea_usage1);
    suite_add_tcase(s, tc_usage);

    TCase *tc_utils = tcase_create("minmea_utils");
    tcase_add_test(tc_utils, test_minmea_gettimeofday);
    tcase_add_test(tc_utils, test_minmea_rescale);
    tcase_add_test(tc_utils, test_minmea_float);
    tcase_add_test(tc_utils, test_minmea_coord);
    suite_add_tcase(s, tc_utils);

    return s;
}

int main()
{
    int number_failed;
    Suite *s = minmea_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* vim: set ts=4 sw=4 et: */
