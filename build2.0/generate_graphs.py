import pandas as pd
import matplotlib.pyplot as plt

# Configuración general de los gráficos
plt.style.use('ggplot')       # Estilo similar a R
plt.rcParams['figure.figsize'] = [12, 6]

def load_data():
    # Cargar los datos de los CSVs
    ic_stats = pd.read_csv('interconnect_report.csv')
    pe_stats = pd.read_csv('pe_stats.csv')
    
    # Extraer valores clave del interconnect
    total_bytes = ic_stats[ic_stats['Metric'] == 'Total Bytes Transferred']['Value'].values[0]
    total_time = ic_stats[ic_stats['Metric'] == 'Total Processing Time (sec)']['Value'].values[0]
    
    # Calcular ancho de banda promedio (bytes/segundo)
    bandwidth = total_bytes / total_time if total_time > 0 else 0
    
    return ic_stats, pe_stats, bandwidth

def plot_bandwidth_utilization(ic_stats, bandwidth):
    # Gráfico 1: Ancho de banda y utilización
    fig, ax1 = plt.subplots()
    
    # Barras para mensajes por tipo
    msg_data = ic_stats[ic_stats['Metric'].str.contains('Messages')]
    msg_types = msg_data['Metric'].str.replace(' Messages', '')
    counts = msg_data['Value']
    
    ax1.bar(msg_types, counts, color='skyblue', alpha=0.7)
    ax1.set_xlabel('Tipo de Mensaje')
    ax1.set_ylabel('Cantidad', color='skyblue')
    ax1.tick_params(axis='y', labelcolor='skyblue')
    
    # Línea para ancho de banda
    ax2 = ax1.twinx()
    ax2.axhline(y=bandwidth, color='red', linestyle='--', linewidth=2)
    ax2.text(0.5, bandwidth*1.05, f'Bandwidth: {bandwidth:.2f} bytes/seg', 
             color='red', ha='center')
    ax2.set_ylabel('Bytes/segundo', color='red')
    ax2.tick_params(axis='y', labelcolor='red')
    
    plt.title('Distribución de Mensajes y Ancho de Banda del Interconnect')
    plt.xticks(rotation=45)
    plt.tight_layout()
    plt.savefig('bandwidth_utilization.png')
    plt.close()

def plot_pe_activity(pe_stats):
    fig, axes = plt.subplots(3, 1, figsize=(12, 12))
    
    # Gráfico 1: Instrucciones ejecutadas
    axes[0].bar(pe_stats['PE_ID'], pe_stats['InstructionsExecuted'], color='#4c72b0')
    axes[0].set_title('Instrucciones Ejecutadas por PE')
    
    # Gráfico 2: Bytes enviados
    axes[1].bar(pe_stats['PE_ID'], pe_stats['BytesSent'], color='#55a868')
    axes[1].set_title('Bytes Enviados por PE')
    
    # Gráfico 3: Tiempo de espera
    axes[2].bar(pe_stats['PE_ID'], pe_stats['WaitTimeSec'], color='#c44e52')
    axes[2].set_title('Tiempo de Espera por PE (segundos)')
    
    plt.tight_layout()
    plt.savefig('pe_activity_separate.png')
    plt.close()

def plot_message_type_distribution(ic_stats):
    # Gráfico 3: Distribución de tipos de mensaje
    # Filtrar solo los tipos de mensaje (excluyendo el total)
    msg_data = ic_stats[
        ic_stats['Metric'].str.contains('Messages') & 
        ~ic_stats['Metric'].str.contains('Total Messages')
    ]
    
    msg_types = msg_data['Metric'].str.replace(' Messages', '')
    counts = msg_data['Value']
    
    # Gráfico de torta
    plt.figure()
    plt.pie(counts, labels=msg_types, autopct='%1.1f%%',
            colors=['#ff9999','#66b3ff','#99ff99','#ffcc99', '#dddddd', '#ffaaaa', '#aaffaa'],
            startangle=90, wedgeprops=dict(width=0.4))
    
    plt.title('Distribución de Tipos de Mensaje en el Interconnect\n')
    plt.tight_layout()
    plt.savefig('message_distribution.png')
    plt.close()

def main():
    ic_stats, pe_stats, bandwidth = load_data()
    
    print(f"\nAncho de banda promedio del sistema: {bandwidth:.2f} bytes/segundo\n")
    
    plot_bandwidth_utilization(ic_stats, bandwidth)
    plot_pe_activity(pe_stats)
    plot_message_type_distribution(ic_stats)
    
    print("Gráficos generados:")
    print("- bandwidth_utilization.png")
    print("- pe_activity.png")
    print("- message_distribution.png")

if __name__ == "__main__":
    main()