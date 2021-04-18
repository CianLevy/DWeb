import copy
import math
import os
import sys

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from matplotlib import rcParams

rcParams.update({'figure.autolayout': True})

def plot_error_bar(hits_dataset, ax, y_label='', x_label='Cache budget', label='', x_err=False, rows=True, colour=None):
    if rows:
        dataset = np.array(hits_dataset).T
    else:
        dataset = hits_dataset

    if x_err:
        ax.errorbar(dataset[0], dataset[1], xerr=dataset[2], yerr=dataset[3], label=label)
    else:
        if colour:
            ax.errorbar(dataset[0], dataset[1], yerr=dataset[2], label=label, c=colour)
        else:
            ax.errorbar(dataset[0], dataset[1], yerr=dataset[2], label=label)
    
    ax.set_xlabel(x_label)
    ax.set_ylabel(y_label)
    # ax.set_title(title)
    ax.legend()
    

def create_new_figure(title):
    fig = plt.figure()
    new_ax = fig.gca()
    new_ax.set_title(title)
    new_ax.grid()
    # plt.tight_layout()

    return new_ax


def plot_all_strategies(dataset, strategy_labels, dataset_name, y_label, ax, rows=True):
    for strategy in dataset.keys():
        plot_error_bar(dataset[strategy][dataset_name], ax, y_label=y_label, label=strategy_labels[strategy], rows=rows)


SUMMARY_FOLDER = 'test_summaries'

def main():
    sub_folder = ''
    csv_file = ''
    node_count = 1


    if len(sys.argv) > 2:
        node_count = int(sys.argv[1])
        sub_folder = sys.argv[2]

        if not os.path.exists(f"{FIGS_DIR}/{sub_folder}"):
            os.makedirs(f"{FIGS_DIR}/{sub_folder}")

        csv_file = f"{SUMMARY_FOLDER}/summary_{sub_folder}.csv"
        sub_folder += '/'


    if len(sys.argv) == 4:
        csv_file = sys.argv[3] + ".csv"

    data = pd.read_csv(csv_file, comment='#')
    datasets = convert_to_datasets(data, node_count)

    labels = {
        'lru': 'LCE (LRU eviction)',
        'magic': 'MAGIC',
        'popularity': 'LCE (Popularity eviction)',
        'dweb_broadcast': 'DWeb Broadcast'
    }

    colours = {
        'lru': u'#1f77b4',
        'magic': u'#ff7f0e',
        'popularity': u'#2ca02c',
        'dweb_broadcast': u'#d62728'
    }

    ax = create_new_figure(f'Average number of cache operations vs Cache budget ({node_count} nodes)')
    plot_all_strategies(datasets, labels, 'total_operations', 'Cache operations (per node)', ax, False)
    plt.savefig(f"{FIGS_DIR}/{sub_folder}operations_plot.png")

    ax = create_new_figure(f'Cache hit ratio vs Cache budget ({node_count} nodes)')
    plot_all_strategies(datasets, labels, 'hit_ratio', 'Hit Ratio', ax, False)
    plt.savefig(f"{FIGS_DIR}/{sub_folder}hit_ratio_plot.png")


    ax = create_new_figure(f'Average hop count vs Cache budget ({node_count} nodes)')
    plot_all_strategies(datasets, labels, 'hop_count', 'Hop Count', ax, False)
    plt.savefig(f"{FIGS_DIR}/{sub_folder}hop_count_plot.png")

    ax = create_new_figure(f'Relative performance: MAGIC vs other strategies ({node_count} nodes)')

    optional_strategies = ['lru', 'popularity', 'dweb_broadcast']

    for strategy in optional_strategies:
        if strategy in datasets:
            hit_ratio_diff = get_relative_diff_dataset(datasets, 'hit_ratio', 'magic', strategy)
            plot_error_bar(hit_ratio_diff, ax, y_label='% increase in hit ratio', 
                           label=f'Relative increase MAGIC vs {labels[strategy]}', rows=False,
                           colour=colours[strategy])

    plt.savefig(f"{FIGS_DIR}/{sub_folder}difference_plot.png")


    ax = create_new_figure('Hit ratio vs Hop count: MAGIC vs LRU')
    new_dataset = get_hit_ratio_vs_hop_count(datasets, labels.keys())

    average_hop_diff = 1 - sum(new_dataset['magic'][1]) / sum(new_dataset['lru'][1])

    print(f"Average MAGIC hop count reduction {average_hop_diff:.4f}")
    plot_error_bar(new_dataset['lru'], ax, 'Hop Count', 'Hit ratio', label=labels['lru'], x_err=True, rows=False)
    plot_error_bar(new_dataset['magic'], ax, 'Hop Count', 'Hit ratio', label=labels['magic'], x_err=True, rows=False)
    plt.savefig(f"{FIGS_DIR}/{sub_folder}hit_ratio_vs_hop_count_plot.png")

    print_summary(datasets, labels)

def print_summary(datasets, labels):
    for name, dataset in datasets.items():
        print(f"{labels[name]}: {dataset['hit_ratio'][1].mean()}")


def convert_to_datasets(data, node_count):
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

    optional_strategies = ['popularity', 'dweb_broadcast']

    for strategy in optional_strategies:
        if strategy in data['strategy'].to_list():
            strategies.append(strategy)
            results[strategy] = copy.deepcopy(datasets)

    for strategy in strategies:
        sub_table = data.loc[data['strategy'] == strategy]

        result = sub_table.groupby(['cache_budget'], as_index=False).agg(['mean','std'])

        results[strategy]['total_operations'].append(result.index.to_list())
        # results[strategy]['total_operations'].append(( result[('evict_count', 'mean')]).to_list())
        # results[strategy]['total_operations'].append((result[('evict_count', 'std')]).to_list())
        results[strategy]['total_operations'].append(((result[('insert_count', 'mean')] + result[('evict_count', 'mean')]) / node_count).to_list())
        results[strategy]['total_operations'].append(((result[('insert_count', 'std')] + result[('evict_count', 'std')]) / node_count).to_list())


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
    
    dataset = np.array(dataset).T

    if dataset[0][1] > 1000:
        dataset = dataset[1:]

    return dataset.T


def get_hit_ratio_vs_hop_count(datasets, strategies):
    results = {}

    for strategy in strategies:
        results[strategy] = [
            datasets[strategy]['hit_ratio'][1],
            datasets[strategy]['hop_count'][1],
            datasets[strategy]['hit_ratio'][2], # std
            datasets[strategy]['hop_count'][2], # std
        ]

    return results


FIGS_DIR = 'images'

if __name__ == '__main__':
    main()
