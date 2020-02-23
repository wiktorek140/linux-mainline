//
// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2020, The Linux Foundation. All rights reserved.
//
#ifndef PANEL_SAMSUNG_S6E3FA7_AMS604NL01_GAMMA_H
#define PANEL_SAMSUNG_S6E3FA7_AMS604NL01_GAMMA_H

struct s6e3fa7_gamma_ctx;

struct s6e3fa7_gamma_ctx *s6e3fa7_gamma_init(char* mtp_data, int mtp_len);

int s6e3fa7_get_gamma(struct s6e3fa7_gamma_ctx *ctx, int gamma_idx, char* output, int output_len);

int s6e3fa7_generate_gamma(struct s6e3fa7_gamma_ctx *ctx, int gamma_idx, char* output, int output_len);

void s6e3fa7_gamma_destroy(struct s6e3fa7_gamma_ctx *ctx);

#endif /* PANEL_SAMSUNG_S6E3FA7_AMS604NL01_GAMMA_H */
