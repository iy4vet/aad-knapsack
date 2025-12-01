import subprocess
import sys
import json
import os
import glob
import random
import argparse
import numpy as np
import matplotlib
matplotlib.use('Agg') # Use non-interactive backend
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import cv2

# Configuration
ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
CODE_DIR = os.path.join(ROOT_DIR, 'code')
ALGORITHMS = ['algo1base', 'algo2custom', 'algo3testbed', 'algo4time']

CASES_DIR = os.path.join(ROOT_DIR, 'data', 'cases')
OUTPUT_DIR = os.path.join(os.path.dirname(__file__), 'output')
os.makedirs(OUTPUT_DIR, exist_ok=True)

def pick_random_case():
    files = glob.glob(os.path.join(CASES_DIR, '*.txt'))
    if not files:
        raise FileNotFoundError(f"No case files found in {CASES_DIR}")
    return random.choice(files)

def compile_code():
    print("Compiling code...")
    try:
        subprocess.check_call(['make'], cwd=ROOT_DIR, shell=True)
    except subprocess.CalledProcessError:
        print("Make failed. Trying direct compilation...")
        # Fallback compilation for all algos
        for algo in ALGORITHMS:
            src = os.path.join(CODE_DIR, f'{algo}.cpp')
            dst = os.path.join(CODE_DIR, f'bin/{algo}')
            if os.name == 'nt': dst += '.exe'
            cmd = ['g++', '-O3', '-std=c++17', src, '-o', dst]
            print(f"Compiling {algo}...")
            subprocess.check_call(cmd)
    print("Compilation successful.")

def run_and_record(algo_name, data_file):
    bin_path = os.path.join(CODE_DIR, f'bin/{algo_name}')
    if os.name == 'nt':
        bin_path += '.exe'

    if not os.path.exists(bin_path):
        print(f"Error: Binary {bin_path} not found.")
        return

    case_name = os.path.basename(data_file).replace('.txt', '')
    video_path = os.path.join(OUTPUT_DIR, f'vis_{algo_name}_{case_name}.mp4')
    
    cmd = [bin_path, '--visualize']
    print(f"Running {algo_name}: {' '.join(cmd)} < {data_file}")
    print(f"Output video: {video_path}")


    # Setup Plot
    fig = plt.figure(figsize=(19.2, 10.8), facecolor='#1e1e1e') 
    gs = fig.add_gridspec(2, 2, height_ratios=[3, 1], width_ratios=[2, 1])
    
    # Main Plot: 3D Gene Heatmap (Weight vs Value vs Frequency)
    ax_main = fig.add_subplot(gs[0, :], projection='3d', facecolor='#1e1e1e')
    ax_main.set_facecolor('#1e1e1e')
    # Hide panes
    ax_main.xaxis.pane.fill = False
    ax_main.yaxis.pane.fill = False
    ax_main.zaxis.pane.fill = False
    ax_main.grid(True, color='#444444', linestyle='--', alpha=0.5)
    
    ax_main.tick_params(axis='x', colors='white')
    ax_main.tick_params(axis='y', colors='white')
    ax_main.tick_params(axis='z', colors='white')
    
    ax_main.set_xlabel('Item Weight', color='white', fontsize=12)
    ax_main.set_ylabel('Item Value', color='white', fontsize=12)
    ax_main.set_zlabel('Selection Freq', color='white', fontsize=12)
    
    # Fitness Hist (Bottom Left)
    ax_hist = fig.add_subplot(gs[1, 0], facecolor='#1e1e1e')
    ax_hist.tick_params(axis='x', colors='white')
    ax_hist.tick_params(axis='y', colors='white')
    ax_hist.set_xlabel('Fitness Value', color='white')
    ax_hist.set_ylabel('Count', color='white')
    ax_hist.set_title('Population Fitness Distribution', color='white')
    for spine in ax_hist.spines.values():
        spine.set_edgecolor('#444444')

    # Stats Line (Bottom Right)
    ax_line = fig.add_subplot(gs[1, 1], facecolor='#1e1e1e')
    ax_line.tick_params(axis='x', colors='white')
    ax_line.tick_params(axis='y', colors='white')
    ax_line.set_xlabel('Generation', color='white')
    ax_line.set_ylabel('Best Value', color='white')
    ax_line.set_title('Convergence', color='white')
    for spine in ax_line.spines.values():
        spine.set_edgecolor('#444444')

    # Video Writer
    # 1920x1080
    fourcc = cv2.VideoWriter_fourcc(*'mp4v')
    out = cv2.VideoWriter(video_path, fourcc, 10.0, (1920, 1080))

    capacity = 0
    items_weights = []
    items_values = []
    
    history_best = []
    history_gens = []
    
    with open(data_file, 'r') as f:
        process = subprocess.Popen(
            cmd,
            stdin=f,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1
        )

        for line in process.stdout:
            if not line.startswith('VIS:'):
                continue
            
            parts = line.strip().split(':', 2)
            if len(parts) < 3: continue
            
            msg_type = parts[1]
            content = json.loads(parts[2])
            
            if msg_type == 'INIT':
                capacity = content.get('capacity', 0)
                if 'weights' in content:
                    items_weights = np.array(content.get('weights', []))
                    items_values = np.array(content.get('values', []))
                elif 'items' in content:
                    # items is [[id, w, v, ...], ...]
                    items = np.array(content['items'])
                    if len(items) > 0:
                        items_weights = items[:, 1]
                        items_values = items[:, 2]
                
            elif msg_type == 'GEN':
                gen = content['gen']
                best_val = content['best_val']
                pop_size = content['pop_size']
                fitness = np.array(content['fitness'])
                freq = np.array(content.get('freq', []))
                
                history_best.append(best_val)
                history_gens.append(gen)
                
                # 1. Update Main 3D Plot
                ax_main.clear()
                ax_main.set_facecolor('#1e1e1e')
                ax_main.grid(True, color='#444444', linestyle='--', alpha=0.5)
                
                if len(items_weights) > 0 and len(freq) > 0:
                    # Scatter plot: X=Weight, Y=Value, Z=Frequency
                    scat = ax_main.scatter(
                        items_weights, 
                        items_values, 
                        freq, 
                        c=freq, 
                        cmap='inferno', 
                        s=30, 
                        alpha=0.8,
                        edgecolors='none'
                    )
                    
                    ax_main.set_zlim(0, pop_size)
                    ax_main.view_init(elev=30, azim=45 + (gen * 0.5))

                ax_main.set_title(f"[{algo_name}] Gen {gen} | Best: {best_val} | Pop: {pop_size}", color='white', fontsize=16)
                ax_main.set_xlabel('Item Weight', color='white')
                ax_main.set_ylabel('Item Value', color='white')
                ax_main.set_zlabel('Selection Freq', color='white')
                ax_main.tick_params(colors='white')
                for spine in ax_main.spines.values(): spine.set_edgecolor('#444444')

                # 2. Update Histogram
                ax_hist.clear()
                # Use a fixed number of bins or fixed width if possible
                # To avoid "shifting", we can try to anchor the bins
                # But since fitness grows, we must shift.
                # We just ensure the formatting is clean.
                
                ax_hist.hist(fitness, bins=20, color='skyblue', alpha=0.7, edgecolor='white')
                ax_hist.set_xlabel('Fitness', color='white')
                ax_hist.set_ylabel('Count', color='white')
                ax_hist.set_title('Fitness Distribution', color='white')
                ax_hist.tick_params(colors='white')
                
                # Force integer formatting for x-axis if values are large
                ax_hist.xaxis.set_major_formatter(matplotlib.ticker.FormatStrFormatter('%d'))
                
                for spine in ax_hist.spines.values(): spine.set_edgecolor('#444444')
                
                # 3. Update Line Plot
                ax_line.clear()
                ax_line.plot(history_gens, history_best, 'g-', linewidth=2)
                ax_line.set_xlabel('Gen', color='white')
                ax_line.set_ylabel('Best Val', color='white')
                ax_line.set_title('Convergence', color='white')
                ax_line.tick_params(colors='white')
                for spine in ax_line.spines.values(): spine.set_edgecolor('#444444')
                
                # Render to image
                fig.canvas.draw()
                
                # Convert to numpy array for OpenCV
                # Modern matplotlib (3.8+)
                try:
                    img = np.frombuffer(fig.canvas.buffer_rgba(), dtype=np.uint8)
                except AttributeError:
                    # Fallback for older versions
                    img = np.frombuffer(fig.canvas.tostring_argb(), dtype=np.uint8)
                    # ARGB -> RGBA conversion might be needed or just use RGB
                
                img = img.reshape(fig.canvas.get_width_height()[::-1] + (4,))
                
                # RGBA to BGR
                img = cv2.cvtColor(img, cv2.COLOR_RGBA2BGR)
                
                out.write(img)
                print(f"[{algo_name}] Processed Gen {gen}", end='\r')

    process.wait()
    out.release()
    plt.close(fig)
    print(f"\nVideo saved to {video_path}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--all', action='store_true', help='Run all algorithms')
    parser.add_argument('--case', type=str, help='Specific case file to run')
    args = parser.parse_args()

    compile_code()
    
    if args.case:
        data_file = args.case
    else:
        data_file = pick_random_case()
        
    print(f"Using case: {data_file}")

    if args.all:
        algos_to_run = ALGORITHMS
    else:
        algos_to_run = ['algo3testbed']
        
    for algo in algos_to_run:
        run_and_record(algo, data_file)

