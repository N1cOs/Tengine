/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * License); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * AS IS BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*
 * Copyright (c) 2020, OPEN AI LAB
 * Author: xwwang@openailab.com
 */

#include <math.h>
#include "sys_port.h"
#include "module.h"
#include "tengine_ir.h"
#include "../../cpu_node_ops.h"
#include "tengine_op.h"
#include "lstm_param.h"

int ref_lstm_default_fp32(struct ir_tensor* input_tensor, struct ir_tensor* w, struct ir_tensor* r, struct ir_tensor* output_tensor, struct lstm_param* param)
{
    int batch_size = input_tensor->dims[1];
    int hidden_size = param->hidden_size;

    float* x_data = input_tensor->data;
    float* w_data = w->data;
    float* r_data = r->data;

    /* initial h, initial c buffers */
    float* init_h_data = (float*)malloc((unsigned long)hidden_size * batch_size * sizeof(float));
    float* init_c_data = (float*)malloc((unsigned long)hidden_size * batch_size * sizeof(float));
    float* output_h_data = (float*)malloc((unsigned long)hidden_size * batch_size * sizeof(float));
    float* output_c_data = (float*)malloc((unsigned long)hidden_size * batch_size * sizeof(float));

    memset(init_h_data, 0, (unsigned long)hidden_size*batch_size * sizeof(float));
    memset(init_c_data, 0, (unsigned long)hidden_size*batch_size * sizeof(float));
    memset(output_h_data, 0, (unsigned long)hidden_size*batch_size * sizeof(float));
    memset(output_c_data, 0, (unsigned long)hidden_size*batch_size * sizeof(float));
    
    float* output_data = output_tensor->data;
    int T = input_tensor->dims[1];
    int size = input_tensor->dims[2];

    float* i_flag = ( float* )malloc(hidden_size * sizeof(float));
    float* f_flag = ( float* )malloc(hidden_size * sizeof(float));
    float* o_flag = ( float* )malloc(hidden_size * sizeof(float));
    float* g_flag = ( float* )malloc(hidden_size * sizeof(float));

    for(int seq = 0; seq < input_tensor->dims[0]; seq++)
    {
        for(int i = 0; i < T; i++)
        {
            for (int q = 0; q < hidden_size; q++)
            {
                float I = 0;
                float F = 0;
                float O = 0;
                float G = 0;
                for (int m = 0; m < size; m++)
                {
                    int index = seq * (input_tensor->dims[1] * input_tensor->dims[2]) + i * input_tensor->dims[2] + m;
                    float x_i = x_data[index];
                    I += x_i * w_data[(hidden_size * 0 + q) * input_tensor->dims[2] + m];
                    O += x_i * w_data[(hidden_size * 1 + q) * input_tensor->dims[2] + m];
                    F += x_i * w_data[(hidden_size * 2 + q) * input_tensor->dims[2] + m];
                    G += x_i * w_data[(hidden_size * 3 + q) * input_tensor->dims[2] + m];
                }

                for (int h = 0; h < hidden_size; h++)
                {
                    if(seq == 0)
                    {
                        float h_i = init_h_data[h + i * hidden_size];
                        I += h_i * (r_data[(hidden_size * 0 + q) * hidden_size + h]);
                        O += h_i * (r_data[(hidden_size * 1 + q) * hidden_size + h]);
                        F += h_i * (r_data[(hidden_size * 2 + q) * hidden_size + h]);
                        G += h_i * (r_data[(hidden_size * 3 + q) * hidden_size + h]);
                    }
                    else
                    {
                        float h_i = output_h_data[h + i * hidden_size];
                        I += h_i * (r_data[(hidden_size * 0 + q) * hidden_size + h]);
                        O += h_i * (r_data[(hidden_size * 1 + q) * hidden_size + h]);
                        F += h_i * (r_data[(hidden_size * 2 + q) * hidden_size + h]);
                        G += h_i * (r_data[(hidden_size * 3 + q) * hidden_size + h]);
                    }
                }

                i_flag[q] = I;
                f_flag[q] = F;
                o_flag[q] = O;
                g_flag[q] = G;
            }

            for (int c = 0; c < hidden_size; c++)
            {
                if( seq == 0)
                {
                    float I = 1.f / (1.f + exp(-i_flag[c]));
                    float F = 1.f / (1.f + exp(-f_flag[c]));
                    float G = tanh(g_flag[c]);
                    float c_i = init_c_data[c + i * hidden_size];
                    float cell2 = F * c_i + I * G;
                    float O = 1.f/(1.f + exp(-o_flag[c]));
                    float tmp = tanh(cell2);
                    float H = O * tmp;
                    output_c_data[i * hidden_size + c] = cell2;
                    output_h_data[i * hidden_size + c] = H;
                    output_data[i * hidden_size + c] = H;
                }
                else
                {
                    float I = 1.f / (1.f + exp(-i_flag[c]));
                    float F = 1.f / (1.f + exp(-f_flag[c]));
                    float G = tanh(g_flag[c]);
                    float c_i = output_c_data[c + i * hidden_size];
                    float cell2 = F * c_i + I * G;
                    float O = 1.f/(1.f + exp(-o_flag[c]));
                    float H = O * tanh(cell2);
                    output_c_data[i * hidden_size + c] = cell2;
                    output_h_data[i * hidden_size + c] = H;
                    output_data[i * hidden_size + c] = H;
                }
            }
        }
    }

    free(init_h_data);
    free(init_c_data);
    free(output_h_data);
    free(output_c_data);
    free(i_flag);
    free(f_flag);
    free(o_flag);
    free(g_flag);

    return 0;
}

int ref_lstm_with_bias_fp32(struct ir_tensor* input_tensor, struct ir_tensor* w, struct ir_tensor* r, struct ir_tensor* b, struct ir_tensor* output_tensor, struct lstm_param* param)
{
    int batch_size = input_tensor->dims[1];
    int hidden_size = param->hidden_size;

    float* x_data = input_tensor->data;
    float* w_data = w->data;
    float* r_data = r->data;
    float* b_data = b->data;

    /* initial h, initial c buffers */
    float* init_h_data = (float*)malloc((unsigned long)hidden_size * batch_size * sizeof(float));
    float* init_c_data = (float*)malloc((unsigned long)hidden_size * batch_size * sizeof(float));
    float* output_h_data = (float*)malloc((unsigned long)hidden_size * batch_size * sizeof(float));
    float* output_c_data = (float*)malloc((unsigned long)hidden_size * batch_size * sizeof(float));

    memset(init_h_data, 0, (unsigned long)hidden_size*batch_size * sizeof(float));
    memset(init_c_data, 0, (unsigned long)hidden_size*batch_size * sizeof(float));
    memset(output_h_data, 0, (unsigned long)hidden_size*batch_size * sizeof(float));
    memset(output_c_data, 0, (unsigned long)hidden_size*batch_size * sizeof(float));

    float* output_data = output_tensor->data;

    int T = input_tensor->dims[1];
    int size = input_tensor->dims[2];

    float* i_flag = ( float* )malloc(hidden_size * sizeof(float));
    float* f_flag = ( float* )malloc(hidden_size * sizeof(float));
    float* o_flag = ( float* )malloc(hidden_size * sizeof(float));
    float* g_flag = ( float* )malloc(hidden_size * sizeof(float));

    for(int seq = 0; seq < input_tensor->dims[0]; seq++)
    {
        for(int i = 0; i < T; i++)
        {
            for (int q = 0; q < hidden_size; q++)
            {
                float I = 0;
                float F = 0;
                float O = 0;
                float G = 0;
                for (int m = 0; m < size; m++)
                {
                    int index = seq * (input_tensor->dims[1] * input_tensor->dims[2]) + i * input_tensor->dims[2] + m;
                    float x_i = x_data[index];
                    I += x_i * w_data[(hidden_size * 0 + q) * input_tensor->dims[2] + m];
                    O += x_i * w_data[(hidden_size * 1 + q) * input_tensor->dims[2] + m];
                    F += x_i * w_data[(hidden_size * 2 + q) * input_tensor->dims[2] + m];
                    G += x_i * w_data[(hidden_size * 3 + q) * input_tensor->dims[2] + m];
                }
                I += b_data[hidden_size * 0 + q];
                O += b_data[hidden_size * 1 + q];
                F += b_data[hidden_size * 2 + q];
                G += b_data[hidden_size * 3 + q];
                for (int h = 0; h < hidden_size; h++)
                {
                    if(seq == 0)
                    {
                        float h_i = init_h_data[h + i * hidden_size];
                        I += h_i * (r_data[(hidden_size * 0 + q) * hidden_size + h]);
                        O += h_i * (r_data[(hidden_size * 1 + q) * hidden_size + h]);
                        F += h_i * (r_data[(hidden_size * 2 + q) * hidden_size + h]);
                        G += h_i * (r_data[(hidden_size * 3 + q) * hidden_size + h]);
                    }
                    else
                    {
                        float h_i = output_h_data[h + i * hidden_size];
                        I += h_i * (r_data[(hidden_size * 0 + q) * hidden_size + h]);
                        O += h_i * (r_data[(hidden_size * 1 + q) * hidden_size + h]);
                        F += h_i * (r_data[(hidden_size * 2 + q) * hidden_size + h]);
                        G += h_i * (r_data[(hidden_size * 3 + q) * hidden_size + h]);
                    }
                }
                I += b_data[hidden_size * 4 + hidden_size * 0 + q];
                O += b_data[hidden_size * 4 + hidden_size * 1 + q];
                F += b_data[hidden_size * 4 + hidden_size * 2 + q];
                G += b_data[hidden_size * 4 + hidden_size * 3 + q];

                i_flag[q] = I;
                f_flag[q] = F;
                o_flag[q] = O;
                g_flag[q] = G;
            }

            for (int c = 0; c < hidden_size; c++)
            {
                if( seq == 0)
                {
                    float I = 1.f / (1.f + exp(-i_flag[c]));
                    float F = 1.f / (1.f + exp(-f_flag[c]));
                    float G = tanh(g_flag[c]);
                    float c_i = init_c_data[c + i * hidden_size];
                    float cell2 = F * c_i + I * G;
                    float O = 1.f/(1.f + exp(-o_flag[c]));
                    float tmp = tanh(cell2);
                    float H = O * tmp;
                    output_c_data[i * hidden_size + c] = cell2;
                    output_h_data[i * hidden_size + c] = H;
                    output_data[i * hidden_size + c] = H;
                }
                else
                {
                    float I = 1.f / (1.f + exp(-i_flag[c]));
                    float F = 1.f / (1.f + exp(-f_flag[c]));
                    float G = tanh(g_flag[c]);
                    float c_i = output_c_data[c + i * hidden_size];
                    float cell2 = F * c_i + I * G;
                    float O = 1.f/(1.f + exp(-o_flag[c]));
                    float H = O * tanh(cell2);
                    output_c_data[i * hidden_size + c] = cell2;
                    output_h_data[i * hidden_size + c] = H;
                    output_data[i * hidden_size + c] = H;
                }
            }
        }
    }

    free(init_h_data);
    free(init_c_data);
    free(i_flag);
    free(f_flag);
    free(o_flag);
    free(g_flag);

    return 0;
}

int ref_lstm_with_bias_case1_fp32(struct ir_tensor* input_tensor, struct ir_tensor* w, struct ir_tensor* r, struct ir_tensor* b, struct ir_tensor* output_tensor, struct lstm_param* param)
{
    int sequence_size = input_tensor->dims[0];
    int batch_size = input_tensor->dims[1];
    int size = input_tensor->dims[2];
    int hidden_size = param->hidden_size;

    float* x_data = input_tensor->data;
    float* w_data = w->data;
    float* r_data = r->data;
    float* b_data = b->data;

    /* initial h, initial c buffers */
    float* init_h_data = ( float* )malloc((unsigned long)hidden_size * batch_size * sizeof(float));
    float* init_c_data = ( float* )malloc((unsigned long)hidden_size * batch_size * sizeof(float));
    float* output_h_data = ( float* )malloc((unsigned long)hidden_size * batch_size * sizeof(float));
    float* output_c_data = ( float* )malloc((unsigned long)hidden_size * batch_size * sizeof(float));

    memset(init_h_data, 0, (unsigned long)hidden_size * batch_size * sizeof(float));
    memset(init_c_data, 0, (unsigned long)hidden_size * batch_size * sizeof(float));
    memset(output_h_data, 0, (unsigned long)hidden_size * batch_size * sizeof(float));
    memset(output_c_data, 0, (unsigned long)hidden_size * batch_size * sizeof(float));

    float* output_data = output_tensor->data;

    float* i_flag = ( float* )malloc(hidden_size * sizeof(float));
    float* f_flag = ( float* )malloc(hidden_size * sizeof(float));
    float* o_flag = ( float* )malloc(hidden_size * sizeof(float));
    float* g_flag = ( float* )malloc(hidden_size * sizeof(float));

    for (int seq = 0; seq < sequence_size; seq++)    // sequence
    {
        for (int i = 0; i < batch_size; i++)    // batch
        {
            for (int q = 0; q < hidden_size; q++)    // hidden
            {
                float I = 0;
                float F = 0;
                float O = 0;
                float G = 0;

                /* input fc */
                for (int m = 0; m < size; m++)    // internal size, the same as four fc implement
                {
                    int index = seq * (batch_size * size) + i * size + m;
                    float i_data = x_data[index];
                    I += i_data * w_data[(hidden_size * 0 + q) * size + m];
                    O += i_data * w_data[(hidden_size * 1 + q) * size + m];
                    F += i_data * w_data[(hidden_size * 2 + q) * size + m];
                    G += i_data * w_data[(hidden_size * 3 + q) * size + m];
                }
                I += b_data[hidden_size * 0 + q];
                O += b_data[hidden_size * 1 + q];
                F += b_data[hidden_size * 2 + q];
                G += b_data[hidden_size * 3 + q];

                /* hidden fc */
                for (int h = 0; h < hidden_size; h++)
                {
                    if (seq == 0)
                    {
                        float h_i = init_h_data[h + i * hidden_size];
                        I += h_i * (r_data[(hidden_size * 0 + q) * hidden_size + h]);
                        O += h_i * (r_data[(hidden_size * 1 + q) * hidden_size + h]);
                        F += h_i * (r_data[(hidden_size * 2 + q) * hidden_size + h]);
                        G += h_i * (r_data[(hidden_size * 3 + q) * hidden_size + h]);
                    }
                    else
                    {
                        float h_i = output_h_data[h + i * hidden_size];
                        I += h_i * (r_data[(hidden_size * 0 + q) * hidden_size + h]);
                        O += h_i * (r_data[(hidden_size * 1 + q) * hidden_size + h]);
                        F += h_i * (r_data[(hidden_size * 2 + q) * hidden_size + h]);
                        G += h_i * (r_data[(hidden_size * 3 + q) * hidden_size + h]);
                    }
                }
                I += b_data[hidden_size * 4 + hidden_size * 0 + q];
                O += b_data[hidden_size * 4 + hidden_size * 1 + q];
                F += b_data[hidden_size * 4 + hidden_size * 2 + q];
                G += b_data[hidden_size * 4 + hidden_size * 3 + q];

                i_flag[q] = I;
                f_flag[q] = F;
                o_flag[q] = O;
                g_flag[q] = G;
            }

            for (int c = 0; c < hidden_size; c++)
            {
                if (seq == 0)
                {
                    float I = 1.f / (1.f + exp(-i_flag[c]));
                    float F = 1.f / (1.f + exp(-f_flag[c]));
                    float G = tanh(g_flag[c]);
                    float c_i = init_c_data[c + i * hidden_size];
                    float cell2 = F * c_i + I * G;
                    float O = 1.f / (1.f + exp(-o_flag[c]));
                    float tmp = tanh(cell2);
                    float H = O * tmp;
                    output_c_data[i * hidden_size + c] = cell2;
                    output_h_data[i * hidden_size + c] = H;
                    output_data[seq * hidden_size * batch_size + i * hidden_size + c] = H;
                }
                else
                {
                    float I = 1.f / (1.f + exp(-i_flag[c]));
                    float F = 1.f / (1.f + exp(-f_flag[c]));
                    float G = tanh(g_flag[c]);
                    float c_i = output_c_data[c + i * hidden_size];
                    float cell2 = F * c_i + I * G;
                    float O = 1.f / (1.f + exp(-o_flag[c]));
                    float H = O * tanh(cell2);
                    output_c_data[i * hidden_size + c] = cell2;
                    output_h_data[i * hidden_size + c] = H;
                    output_data[seq * hidden_size * batch_size + i * hidden_size + c] = H;
                }
            }
        }
    }

    free(init_h_data);
    free(init_c_data);
    free(output_h_data);
    free(output_c_data);
    free(i_flag);
    free(f_flag);
    free(o_flag);
    free(g_flag);

    return 0;
}

int ref_lstm_with_peepholes_fp32(struct ir_tensor* input_tensor, struct ir_tensor* w, struct ir_tensor* r, 
                                struct ir_tensor* b, struct ir_tensor* sequence_lens, struct ir_tensor* init_h, struct ir_tensor* init_c, struct ir_tensor* p, 
                                struct ir_tensor* output_tensor, struct lstm_param* param)
{
    int batch_size = input_tensor->dims[1];
    int hidden_size = param->hidden_size;

    float* x_data = input_tensor->data;
    float* w_data = w->data;
    float* r_data = r->data;
    float* b_data = b->data;
    float* init_h_data = init_h->data;
    float* init_c_data = init_c->data;
    float* p_data = p->data;
    
    float* output_data = output_tensor->data;

    float* output_h_data = (float*)malloc((unsigned long)hidden_size * batch_size * sizeof(float));
    float* output_c_data = (float*)malloc((unsigned long)hidden_size * batch_size * sizeof(float));
    memset(output_h_data, 0, (unsigned long)hidden_size*batch_size * sizeof(float));
    memset(output_c_data, 0, (unsigned long)hidden_size*batch_size * sizeof(float));

    int T = input_tensor->dims[1];
    int size = input_tensor->dims[2];

    float* i_flag = ( float* )malloc(hidden_size * sizeof(float));
    float* f_flag = ( float* )malloc(hidden_size * sizeof(float));
    float* o_flag = ( float* )malloc(hidden_size * sizeof(float));
    float* g_flag = ( float* )malloc(hidden_size * sizeof(float));

    for(int seq = 0; seq < input_tensor->dims[0]; seq++)
    {
        for(int i = 0; i < T; i++)
        {
            for (int q = 0; q < hidden_size; q++)
            {
                float I = 0;
                float F = 0;
                float O = 0;
                float G = 0;
                for (int m = 0; m < size; m++)
                {
                    int index = seq * (input_tensor->dims[1] * input_tensor->dims[2]) + i * input_tensor->dims[2] + m;
                    float x_i = x_data[index];
                    I += x_i * w_data[(hidden_size * 0 + q) * input_tensor->dims[2] + m];
                    O += x_i * w_data[(hidden_size * 1 + q) * input_tensor->dims[2] + m];
                    F += x_i * w_data[(hidden_size * 2 + q) * input_tensor->dims[2] + m];
                    G += x_i * w_data[(hidden_size * 3 + q) * input_tensor->dims[2] + m];
                }
                I += b_data[hidden_size * 0 + q];
                O += b_data[hidden_size * 1 + q];
                F += b_data[hidden_size * 2 + q];
                G += b_data[hidden_size * 3 + q];
                for (int h = 0; h < hidden_size; h++)
                {
                    if(seq == 0)
                    {
                        float h_i = init_h_data[h + i * hidden_size];
                        I += h_i * (r_data[(hidden_size * 0 + q) * hidden_size + h]);
                        O += h_i * (r_data[(hidden_size * 1 + q) * hidden_size + h]);
                        F += h_i * (r_data[(hidden_size * 2 + q) * hidden_size + h]);
                        G += h_i * (r_data[(hidden_size * 3 + q) * hidden_size + h]);
                    }
                    else
                    {
                        float h_i = output_h_data[h + i * hidden_size];
                        I += h_i * (r_data[(hidden_size * 0 + q) * hidden_size + h]);
                        O += h_i * (r_data[(hidden_size * 1 + q) * hidden_size + h]);
                        F += h_i * (r_data[(hidden_size * 2 + q) * hidden_size + h]);
                        G += h_i * (r_data[(hidden_size * 3 + q) * hidden_size + h]);
                    }
                }
                I += b_data[hidden_size * 4 + hidden_size * 0 + q];
                O += b_data[hidden_size * 4 + hidden_size * 1 + q];
                F += b_data[hidden_size * 4 + hidden_size * 2 + q];
                G += b_data[hidden_size * 4 + hidden_size * 3 + q];

                i_flag[q] = I;
                f_flag[q] = F;
                o_flag[q] = O;
                g_flag[q] = G;
            }

            for (int c = 0; c < hidden_size; c++)
            {
                if( seq == 0)
                {
                    float I = 1.f / (1.f + exp(-i_flag[c]));
                    float F = 1.f / (1.f + exp(-f_flag[c]));
                    float G = tanh(g_flag[c]);
                    float c_i = init_c_data[c + i * hidden_size];
                    float cell2 = F * c_i + I * G;
                    float O = 1.f/(1.f + exp(-(o_flag[c] + p_data[0 * hidden_size + c] * cell2)));
                    float tmp = tanh(cell2);
                    float H = O * tmp;
                    output_c_data[i * hidden_size + c] = cell2;
                    output_h_data[i * hidden_size + c] = H;
                    output_data[i * hidden_size + c] = H;
                }
                else
                {
                    float I = 1.f / (1.f + exp(-(i_flag[c] + p_data[0 * hidden_size + c] * output_c_data[c])));
                    float F = 1.f / (1.f + exp(-(f_flag[c] + p_data[1 * hidden_size + c] * output_c_data[c])));
                    float G = tanh(g_flag[c]);
                    float c_i = output_c_data[c + i * hidden_size];
                    float cell2 = F * c_i + I * G;
                    float O = 1.f/(1.f + exp(-(o_flag[c] + p_data[2 * hidden_size + c] * cell2)));
                    float H = O * tanh(cell2);
                    output_c_data[i * hidden_size + c] = cell2;
                    output_h_data[i * hidden_size + c] = H;
                    output_data[i * hidden_size + c] = H;
                }
            }
        }
    }

    free(output_h_data);
    free(output_c_data);
    free(i_flag);
    free(f_flag);
    free(o_flag);
    free(g_flag);
    
    return 0;    
}

int ref_lstm_with_bias_bidirection_fp32(struct ir_tensor* input_tensor, struct ir_tensor* w, struct ir_tensor* r, struct ir_tensor* b, struct ir_tensor* output_tensor, struct lstm_param* param)
{
    int batch_size = input_tensor->dims[1];
    int hidden_size = param->hidden_size;

    float* x_data = input_tensor->data;
    float* w_data = w->data;
    float* r_data = r->data;
    float* b_data = b->data;

    /* initial h, initial c buffers */
    float* init_h_data = (float*)malloc(2 * (unsigned long)hidden_size * batch_size * sizeof(float));
    float* init_c_data = (float*)malloc(2 * (unsigned long)hidden_size * batch_size * sizeof(float));
    memset(init_h_data, 0, 2 * (unsigned long)hidden_size * batch_size * sizeof(float));
    memset(init_c_data, 0, 2 * (unsigned long)hidden_size * batch_size * sizeof(float));

    float* output_data = output_tensor->data;
    float* output_h_data = init_h_data;
    float* output_c_data = init_c_data;

    int T = input_tensor->dims[1];
    int size = input_tensor->dims[2];
    int direct_num = input_tensor->dims[0];

    float* i_flag = ( float* )malloc(hidden_size * sizeof(float));
    float* f_flag = ( float* )malloc(hidden_size * sizeof(float));
    float* o_flag = ( float* )malloc(hidden_size * sizeof(float));
    float* g_flag = ( float* )malloc(hidden_size * sizeof(float));

    for(int seq = 0; seq < input_tensor->dims[0]; seq++)
    {
        for(int i = 0; i < T; i++)
        {
            for(int d = 0; d < direct_num; d++)
            {
                for (int q = 0; q < hidden_size; q++)
                {
                    float I = 0;
                    float F = 0;
                    float O = 0;
                    float G = 0;
                    for (int m = 0; m < size; m++)
                    {
                        int index = seq * (input_tensor->dims[1] * input_tensor->dims[2]) + i * input_tensor->dims[2] + m;
                        float x_i = x_data[index];
                        I += x_i * w_data[d * input_tensor->dims[2] * hidden_size * 4 + (hidden_size * 0 + q) * input_tensor->dims[2] + m];
                        O += x_i * w_data[d * input_tensor->dims[2] * hidden_size * 4 + (hidden_size * 1 + q) * input_tensor->dims[2] + m];
                        F += x_i * w_data[d * input_tensor->dims[2] * hidden_size * 4 + (hidden_size * 2 + q) * input_tensor->dims[2] + m];
                        G += x_i * w_data[d * input_tensor->dims[2] * hidden_size * 4 + (hidden_size * 3 + q) * input_tensor->dims[2] + m];
                    }
                    I += b_data[d * hidden_size * 4 * 2 + hidden_size * 0 + q];
                    O += b_data[d * hidden_size * 4 * 2 + hidden_size * 1 + q];
                    F += b_data[d * hidden_size * 4 * 2 + hidden_size * 2 + q];
                    G += b_data[d * hidden_size * 4 * 2 + hidden_size * 3 + q];
                    for (int h = 0; h < hidden_size; h++)
                    {
                        if(seq == 0)
                        {
                            float h_i = init_h_data[d * input_tensor->dims[1] * hidden_size + h + i * hidden_size];
                            I += h_i * (r_data[d * hidden_size * hidden_size * 4 + (hidden_size * 0 + q) * hidden_size + h]);
                            O += h_i * (r_data[d * hidden_size * hidden_size * 4 + (hidden_size * 1 + q) * hidden_size + h]);
                            F += h_i * (r_data[d * hidden_size * hidden_size * 4 + (hidden_size * 2 + q) * hidden_size + h]);
                            G += h_i * (r_data[d * hidden_size * hidden_size * 4 + (hidden_size * 3 + q) * hidden_size + h]);
                        }
                        else
                        {
                            float h_i = output_h_data[d * input_tensor->dims[1] * hidden_size + h + i * hidden_size];
                            I += h_i * (r_data[d * hidden_size * hidden_size * 4 + (hidden_size * 0 + q) * hidden_size + h]);
                            O += h_i * (r_data[d * hidden_size * hidden_size * 4 + (hidden_size * 1 + q) * hidden_size + h]);
                            F += h_i * (r_data[d * hidden_size * hidden_size * 4 + (hidden_size * 2 + q) * hidden_size + h]);
                            G += h_i * (r_data[d * hidden_size * hidden_size * 4 + (hidden_size * 3 + q) * hidden_size + h]);
                        }
                    }
                    I += b_data[d * hidden_size * 4 + hidden_size * 4 + hidden_size * 0 + q];
                    O += b_data[d * hidden_size * 4 + hidden_size * 4 + hidden_size * 1 + q];
                    F += b_data[d * hidden_size * 4 + hidden_size * 4 + hidden_size * 2 + q];
                    G += b_data[d * hidden_size * 4 + hidden_size * 4 + hidden_size * 3 + q];

                    i_flag[q] = I;
                    f_flag[q] = F;
                    o_flag[q] = O;
                    g_flag[q] = G;
                }
                for (int c = 0; c < hidden_size; c++)
                {
                    if( seq == 0)
                    {
                        float I = 1.f / (1.f + exp(-i_flag[c]));
                        float F = 1.f / (1.f + exp(-f_flag[c]));
                        float G = tanh(g_flag[c]);
                        float c_i = init_c_data[d * hidden_size * input_tensor->dims[2] + c + i * hidden_size];
                        float cell2 = F * c_i + I * G;
                        float O = 1.f/(1.f + exp(-o_flag[c]));
                        float tmp = tanh(cell2);
                        float H = O * tmp;
                        output_c_data[d * hidden_size * input_tensor->dims[2] + i * hidden_size + c] = cell2;
                        output_h_data[d * hidden_size * input_tensor->dims[2] + i * hidden_size + c] = H;
                        output_data[seq * 2 * input_tensor->dims[1] * hidden_size + d * hidden_size * input_tensor->dims[2] + i * hidden_size + c] = H;
                    }
                    else
                    {
                        float I = 1.f / (1.f + exp(-i_flag[c]));
                        float F = 1.f / (1.f + exp(-f_flag[c]));
                        float G = tanh(g_flag[c]);
                        float c_i = output_c_data[d * hidden_size * input_tensor->dims[2] + c + i * hidden_size];
                        float cell2 = F * c_i + I * G;
                        float O = 1.f/(1.f + exp(-o_flag[c]));
                        float H = O * tanh(cell2);
                        output_c_data[d * hidden_size * input_tensor->dims[2] + i * hidden_size + c] = cell2;
                        output_h_data[d * hidden_size * input_tensor->dims[2] + i * hidden_size + c] = H;
                        output_data[seq * 2 * input_tensor->dims[1] * hidden_size + d * hidden_size * input_tensor->dims[2] + i * hidden_size + c] = H;
                    }
                }
            }
        }
    }
    free(init_h_data);
    free(init_c_data);
    free(i_flag);
    free(f_flag);
    free(o_flag);
    free(g_flag);

    return 0;
}

static int init_node(struct node_ops* node_ops, struct exec_node* exec_node, struct exec_graph* exec_graph)
{
    return 0;
}

static int release_node(struct node_ops* node_ops, struct exec_node* exec_node, struct exec_graph* exec_graph)
{
    return 0;
}

static int run(struct node_ops* node_ops, struct exec_node* exec_node, struct exec_graph* exec_graph)
{
    // add by wangxinwei for lstm op
    struct ir_node* ir_node = exec_node->ir_node;
    struct ir_graph* ir_graph = ir_node->graph;

    struct ir_tensor* input_tensor = get_ir_graph_tensor(ir_graph, ir_node->input_tensors[0]);
    struct ir_tensor* w = get_ir_graph_tensor(ir_graph, ir_node->input_tensors[1]);
    struct ir_tensor* r = get_ir_graph_tensor(ir_graph, ir_node->input_tensors[2]);
    struct ir_tensor* output_tensor = get_ir_graph_tensor(ir_graph, ir_node->output_tensors[0]);
    
    struct ir_tensor* b = NULL;
    struct ir_tensor* sequence_lens = NULL;
    struct ir_tensor* init_h = NULL;
    struct ir_tensor* init_c = NULL;
    struct ir_tensor* p = NULL;

    lstm_param_t* param = ( struct lstm_param* )(ir_node->op.param_mem);

    /* only support one way */
    if (w->dim_num == 4 && w->dims[0] == 2)
    {
        printf("LSTM only support one way.\n");
        return -1;
    }

    int ret = -1;
    if (ir_node->input_num == 3)
    {
        ret = ref_lstm_default_fp32(input_tensor, w, r, output_tensor, param);
    }
    else if (ir_node->input_num == 4)
    {
        b = get_ir_graph_tensor(ir_graph, ir_node->input_tensors[3]);

        ret = ref_lstm_with_bias_fp32(input_tensor, w, r, b, output_tensor, param);
    }
    else if (ir_node->input_num == 6)
    {
        b = get_ir_graph_tensor(ir_graph, ir_node->input_tensors[3]);

        ret = ref_lstm_with_bias_case1_fp32(input_tensor, w, r, b, output_tensor, param);
    }
    else
    {
        b = get_ir_graph_tensor(ir_graph, ir_node->input_tensors[3]);
        sequence_lens = get_ir_graph_tensor(ir_graph, ir_node->input_tensors[4]);
        init_h = get_ir_graph_tensor(ir_graph, ir_node->input_tensors[5]);
        init_c = get_ir_graph_tensor(ir_graph, ir_node->input_tensors[6]);
        p = get_ir_graph_tensor(ir_graph, ir_node->input_tensors[7]);

        ret = ref_lstm_with_peepholes_fp32(input_tensor, w, r, b, sequence_lens, init_h, init_c, p, output_tensor, param);
    }

    return ret;
}

static int reshape(struct node_ops* node_ops, struct exec_node* exec_node, struct exec_graph* exec_graph)
{
    struct ir_node* node = exec_node->ir_node;
    struct ir_graph* ir_graph = node->graph;
    struct ir_tensor* input = get_ir_graph_tensor(ir_graph, node->input_tensors[0]);
    struct ir_tensor* output = get_ir_graph_tensor(ir_graph, node->output_tensors[0]);
    struct lstm_param* lstm_param = ( struct lstm_param* )(node->op.param_mem);

    int batch_size = input->dims[1];
    if (lstm_param->mxnet_flag == 0)
    {
        batch_size = input->dims[0];
    }
    int dims[4];
    if (lstm_param->mxnet_flag == 0)
    {
        dims[0] = input->dims[0];
        dims[1] = 1;
        dims[2] = input->dims[1];
        dims[3] = lstm_param->hidden_size;
    }
    else
    {
        dims[0] = input->dims[0];
        dims[1] = batch_size;
        dims[2] = lstm_param->hidden_size;
    }

    int ret = set_ir_tensor_shape(output, dims, 4);

    return ret;
}

static int score(struct node_ops* node_ops, struct exec_graph* exec_graph, struct ir_node* exec_node)
{
    return OPS_SCORE_CANDO;
}

static struct node_ops lstm_node_ops = {.prerun = NULL,
                                        .run = run,
                                        .reshape = reshape,
                                        .postrun = NULL,
                                        .init_node = init_node,
                                        .release_node = release_node,
                                        .score = score};

static int reg_lstm_ops(void* arg)
{
    return register_builtin_node_ops(OP_LSTM, &lstm_node_ops);
}

static int unreg_lstm_ops(void* arg)
{
    return unregister_builtin_node_ops(OP_LSTM, &lstm_node_ops);
}

AUTO_REGISTER_OPS(reg_lstm_ops);
AUTO_UNREGISTER_OPS(unreg_lstm_ops);
