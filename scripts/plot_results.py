import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import math
import sys
import os
import copy


def plot_error_bar(hits_dataset, ax, y_label='', label='', rows=True):
    if rows:
        dataset = np.array(hits_dataset).T
    else:
        dataset = hits_dataset

    ax.errorbar(dataset[0], dataset[1], yerr=dataset[2], label=label)
    ax.set_xlabel('Cache budget')
    ax.set_ylabel(y_label)
    # ax.set_title(title)
    ax.legend()
    

def create_new_figure(title):
    fig = plt.figure()
    new_ax = fig.gca()
    new_ax.set_title(title)
    new_ax.grid()

    return new_ax


def plot_all_strategies(dataset, strategy_labels, dataset_name, y_label, ax, rows=True):
    for strategy in dataset.keys():
        plot_error_bar(dataset[strategy][dataset_name], ax, y_label, strategy_labels[strategy], rows)


def main():
    sub_folder = ''
    csv_file = ''

    if len(sys.argv) > 1:
        sub_folder = sys.argv[1]

        if not os.path.exists(f"{FIGS_DIR}/{sub_folder}"):
            os.makedirs(f"{FIGS_DIR}/{sub_folder}")

        csv_file = f"summary_{sub_folder}.csv"
        sub_folder += '/'
        
    if len(sys.argv) == 3:
        csv_file = sys.argv[2] + ".csv"

    data = pd.read_csv(csv_file, comment='#')
    datasets = convert_to_datasets(data)

    labels = {
        'lru': 'LRU',
        'magic': 'MAGIC'
    }

    ax = create_new_figure('Cache budget vs total cache operations')
    plot_all_strategies(datasets, labels, 'total_operations', 'Total cache operations', ax, False)
    plt.savefig(f"{FIGS_DIR}/{sub_folder}operations_plot.png")

    ax = create_new_figure('Cache hit ratio: MAGIC vs LRU (100 node topology)')
    plot_all_strategies(datasets, labels, 'hit_ratio', 'Hit Ratio', ax, False)
    plt.savefig(f"{FIGS_DIR}/{sub_folder}hit_ratio_plot.png")


    ax = create_new_figure('Average hop count: MAGIC vs LRU (100 node topology)')
    plot_all_strategies(datasets, labels, 'hop_count', 'Hop Count', ax, False)
    plt.savefig(f"{FIGS_DIR}/{sub_folder}hop_count_plot.png")


    hit_ratio_diff = get_relative_diff_dataset(datasets, 'hit_ratio', 'magic', 'lru')
    ax = create_new_figure('Percentage increase cache hit ratio MAGIC vs LRU (100 node topology)')
    plot_error_bar(hit_ratio_diff, ax, y_label='% increase in hit ratio', rows=False)
    plt.savefig(f"{FIGS_DIR}/{sub_folder}difference_plot.png")


def convert_to_datasets(data):
    strategies = ['lru', 'magic']
    
    datasets = {
        'total_operations': [],
        'hit_ratio': [],
        'hop_count': [],
    }

    results = {
        'lru': copy.deepcopy(datasets),
        'magic': copy.deepcopy(datasets),
    }

    for strategy in strategies:
        sub_table = data.loc[data['strategy'] == strategy]

        result = sub_table.groupby(['cache_budget'], as_index=False).agg(['mean','std'])

        results[strategy]['total_operations'].append(result.index.to_list())
        results[strategy]['total_operations'].append((result[('insert_count', 'mean')] + result[('evict_count', 'mean')]).to_list())
        results[strategy]['total_operations'].append((result[('insert_count', 'std')] + result[('evict_count', 'std')]).to_list())

        add_dataset(results, 'hit_ratio', strategy, result)
        add_dataset(results, 'hop_count', strategy, result)

    return results


def add_dataset(result_dict, column, strategy, table):
    result_dict[strategy][column].append(table.index.to_list())
    result_dict[strategy][column].append((table[(column, 'mean')]).to_list())
    result_dict[strategy][column].append((table[(column, 'std')]).to_list())


def get_relative_diff_dataset(datasets, dataset_name, strategy_1, strategy_2):
    def convert_to_numpy(d):
        for i in range(len(d)):
            d[i] = np.array(d[i])
        return d

    dataset = []
    dataset_1 = convert_to_numpy(datasets[strategy_1][dataset_name])
    dataset_2 = convert_to_numpy(datasets[strategy_2][dataset_name])

    relative_diff_percentage = (dataset_1[1] / dataset_2[1] - 1) * 100
    diff_error = (dataset_1[2] / dataset_1[1] + dataset_2[2] / dataset_2[1])* 100
    dataset = [dataset_1[0], relative_diff_percentage, diff_error]

    return dataset


FIGS_DIR = 'images'

if __name__ == '__main__':
    main()