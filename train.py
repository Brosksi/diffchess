import torch
import torch.nn as nn
import json
import math
from torch.utils.data import DataLoader
import numpy as np
import bitsandbytes as bnb
import os

d = 'cuda' if torch.cuda.is_available() else 'cpu'

piece_to_id = {
    'P': 0, 'N': 1, 'B': 2, 'R': 3, 'Q': 4, 'K': 5,
    'p': 6, 'n': 7, 'b': 8, 'r': 9, 'q': 10, 'k': 11
}

move = {
    'w':0, 'b':1
}

castle = {'-': 0, 'K': 1, 'Q': 2, 'k': 4, 'q': 8,
            'KQ': 3, 'Kk': 5, 'Kq': 9, 'Qk': 6, 'Qq': 10,
            'kq': 12, 'KQk': 7, 'KQq': 11, 'Kkq': 13, 'Qkq': 14, 'KQkq': 15 
}

m_id = 'move_id.json'
with open (m_id, 'r') as f:
    move_id = json.load(f)


new = 'new.jsonl'
dat = []
with open (new, 'r') as z:
    for line in z:
        data = json.loads(line)
        dat.append(data)

class board_encode(nn.Module):
    def __init__(self, D=256):
        super().__init__()
        self.D = D
        self.piece_emb = nn.Embedding(12, D)
        self.move_emb = nn.Embedding(2, D)
        self.castling_emb = nn.Embedding(16,D)
        self.square_emb = nn.Embedding(64,D)
        self.en_passant_emb = nn.Embedding(65,D)
        self.halfmove_emb = nn.Embedding(101,D)
        
    def en_passant(self, ep):
        if ep == '-':
            return 64
        return (8 - int(ep[1])) * 8 + (ord(ep[0]) - ord('a'))
    
    def forward(self, board):
        parts = board.split(' ')
        board_str, side, castling, ep, halfmove = parts[0], parts[1], parts[2], parts[3], int(parts[4])
        
        tokens = []
        sq_id = 0
        
        for char in board_str:
            if char == '/':
                continue 
            elif char.isdigit():
                for _ in range(int(char)):
                    tokens.append(self.square_emb(torch.tensor(sq_id, device = d)))
                    sq_id += 1
            else:
                pid = piece_to_id[char]
                tokens.append(
                    self.piece_emb(torch.tensor(pid, device = d)) +
                    self.square_emb(torch.tensor(sq_id, device = d)) 
                ) # D * 64 for each square
                sq_id += 1
                
        tokens.append(self.move_emb(torch.tensor(move[side], device = d))) # D
        tokens.append(self.castling_emb(torch.tensor(castle[castling], device = d))) #D
        tokens.append(self.en_passant_emb(torch.tensor(self.en_passant(ep), device = d))) #D
        tokens.append(self.halfmove_emb(torch.tensor(halfmove, device = d))) # D
        
        return torch.stack(tokens) # D+D+D+D+(64*D) = 68*D; size = (68, D)

class move_encode(nn.Module):
    def __init__(self,nm = len(move_id) , D=256, num = 3):
        super().__init__()
        self.m_emb = nn.Embedding(nm, D)
        self.pos_emb = nn.Embedding(num, D)
        
    def forward(self, moves):
        tokens = []
        for i, move in enumerate(moves):
            tokens.append(
                self.m_emb(torch.tensor(move_id[move], device = d))+
                self.pos_emb(torch.tensor(i, device = d))
                )
            
        return torch.stack(tokens)
class noising(nn.Module):
    def __init__(self, T = 1000, s = 0.008):
        super().__init__()
        self.T = T
        self.s = s
        timesteps = torch.arange(T+1, dtype = torch.float32, device = d)
        alpha_bar = torch.cos(
            (timesteps/T+s) / (1+s) * math.pi/2
        )**2
        
        self.register_buffer('alpha_bar', alpha_bar)
        self.register_buffer('sqrt_alpha_bar', torch.sqrt(self.alpha_bar))
        self.register_buffer('one_alpha_bar', torch.sqrt(1 - self.alpha_bar))
        
    def add_noise(self, x0, t, noise = None):
        if noise is None:
            noise = torch.randn_like(x0)
        sqrt_ab = self.sqrt_alpha_bar[t].view(-1, 1,1)
        sqrt_1ab = self.one_alpha_bar[t].view(-1,1,1)
        
        x_t = (sqrt_ab*x0) + (sqrt_1ab*noise)
        return x_t, noise
    
    def sample_time(self, batch_size):
        return torch.randint(0, self.T, (batch_size,), device=d)
class timestep_emb(nn.Module):
    def __init__(self, D = 256):
        super().__init__()
        self.D = D
        self.mlp = nn.Sequential(
            nn.Linear(D, D*4),
            nn.SiLU(),
            nn.Linear(D*4, D),
        )
        
    def forward(self, t):
        half = self.D//2
        freqs = torch.exp(-math.log(10000) * torch.arange(half, device = d)/half)
        args = t.view(-1, 1).float() * freqs.view(1,-1)
        emb = torch.cat([torch.cos(args), torch.sin(args)], dim = -1)
        return self.mlp(emb)

class branch_emb(nn.Module):
    def __init__(self, D = 256, num_branches = 4):
        super().__init__()
        self.num_branches = num_branches
        self.embeddings = nn.Embedding(num_branches, D)
    def forward(self, branch_ids):
        return self.embeddings(branch_ids)
    
class Ada(nn.Module):
    def __init__(self, D = 256):
        super().__init__()
        
        self.mlp = nn.Sequential(
            nn.SiLU(),
            nn.Linear(D, D*9)
        )
        
        nn.init.zeros_(self.mlp[-1].weight)
        nn.init.zeros_(self.mlp[-1].bias)
        
    def forward(self, c):
        params = self.mlp(c).chunk(9, dim=-1)
        return tuple(p.unsqueeze(1) for p in params)

class Expert(nn.Module):
    def __init__(self, D = 512, hidden_dim = 2048):
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(D, hidden_dim),
            nn.SiLU(),
            nn.Linear(hidden_dim, D)
        )
    def forward(self, x):
        return self.net(x)
    
import torch.nn.functional as F
class MoEFFN(nn.Module):
    def __init__(self, D = 512, num_experts=32, top_k = 3, hidden_dim = 2048):
        super().__init__()
        self.num_experts = num_experts
        self.top_k = top_k
    
        self.router = nn.Linear(D*2, num_experts)
        self.experts = nn.ModuleList([
            Expert(D, hidden_dim) for _ in range(num_experts)
        ])
        
    def forward(self, h, c): # h is the horizon moves, and c is global state emb
        B, S, D = h.shape
        c_expanded = c.unsqueeze(1).expand(B, S, D) 
        router_input = torch.cat([h, c_expanded], dim = -1) # B, S, 2D
        logits = self.router(router_input) # B, S, 16
        probs = F.softmax(logits, dim = -1)
        top_k_probs, top_k_idx = probs.topk(self.top_k, dim = -1) # B, S, 2
        top_k_weights = top_k_probs/top_k_probs.sum(dim=-1, keepdim = True)
        
        h_flat = h.view(B*S, D)
        top_k_idx_flat = top_k_idx.view(B*S, self.top_k) 
        top_k_weights_flat = top_k_weights.view(B*S, self.top_k)
        
        out_flat = torch.zeros_like(h_flat, device = d) #(B*S, D)
        
        for i in range(self.num_experts):
            for k in range(self.top_k):
                mask = (top_k_idx_flat[:, k] == i) # maks = (B*S,)
                if mask.any():
                    tokens = h_flat[mask] 
                    expert_out = self.experts[i](tokens)
                    weights = top_k_weights_flat[mask, k] #(num_selected)
                    out_flat[mask] += weights.unsqueeze(-1) * expert_out
                    
        out = out_flat.reshape(B, S, D)
        
        P = probs.mean(dim=(0,1)) # mean activation of each expert across a batch (num_experts,)
        expert_counts = torch.zeros(self.num_experts, device = d) # literal count of its activation (num_experts,)
        for k in range (self.top_k):
            indices_flat = top_k_idx_flat[:, k]
            expert_counts.scatter_add_(0, indices_flat, torch.ones_like(indices_flat, dtype = torch.float32, device = d))
        f = expert_counts/(B*S * self.top_k)
        balance_loss = self.num_experts * (P*f).sum()
        return out, balance_loss  

class DiT(nn.Module):
    def __init__(self, D = 512, num_heads = 16, num_experts = 32, top_k =3):
        super().__init__()
        self.D = D
        self.num_heads = num_heads
        
        self.norm1 = nn.LayerNorm(D, elementwise_affine=False)
        self.norm2 = nn.LayerNorm(D, elementwise_affine=False)
        self. norm3 = nn.LayerNorm(D, elementwise_affine=False)
        
        self.self_attn = nn.MultiheadAttention(D, self.num_heads, batch_first=True)
        self.cross_attn = nn.MultiheadAttention(D, self.num_heads, batch_first=True)
        
        self.adaln = Ada(D)
        
        self.moe = MoEFFN(D, num_experts, top_k, D*4)
        
    def forward(self, h, C, c):
        g1, b1, a1, g2, b2, a2, g3, b3, a3 = self.adaln(c)
        
        h_norm = self.norm1(h) * (1 + g1) + b1
        attn_out, _ = self.self_attn(h_norm, h_norm, h_norm)
        h = h + a1 * attn_out
        
        h_norm = self.norm2(h) * (1 + g2) + b2
        cross_out, _ = self.cross_attn(query = h_norm, key = C, value = C)
        h = h + a2 * cross_out
        
        h_norm = self.norm3(h) * (1+g3) + b3
        moe_out, balance_loss = self.moe(h_norm,c)
        h = h + a3 * moe_out
        return h, balance_loss

class diffusion(nn.Module):
    def __init__(self, D = 512, num_blocks = 24, num_heads = 16, num_experts = 32, top_k=3, num_branches = 4):
        super().__init__()
        self.D = D
        
        self.final_ada = nn.Sequential(
            nn.SiLU(),
            nn.Linear(D, 2*D)
        )
        
        self.time_emb = timestep_emb(D)
        self.branch_emb = branch_emb(D, num_branches=num_branches)
        self.input_proj = nn.Linear(D, D)
        
        self.blocks = nn.ModuleList([
            DiT(D, num_heads, num_experts, top_k) for _ in range(num_blocks)
        ])
        
        self.output_proj = nn.Linear(D, D)
        nn.init.zeros_(self.output_proj.weight)
        nn.init.zeros_(self.output_proj.bias)
        
        self.final_norm = nn.LayerNorm(D, elementwise_affine=False)
        
    def forward(self, x_t, t, C, branch_ids):
        e_t = self.time_emb(t)
        e_b = self.branch_emb(branch_ids)
        
        c_global = C.mean(dim=1)
        c = e_t + e_b + c_global
        h = self.input_proj(x_t)
        total_balance_loss = 0.0
        
        for block in self.blocks:
            h, bl = block(h, C, c)
            total_balance_loss += bl
            
        g_final, b_final= self.final_ada(c).chunk(2, dim=-1)
        h = self.final_norm(h) * (1+g_final.unsqueeze(1)) + b_final.unsqueeze(1)
        v_hat = self.output_proj(h)
        return v_hat, total_balance_loss        



def create_batch(Batch = 256, d = dat):
    idx = np.random.permutation(len(d))
    return [[d[j] for j in idx[b:b+Batch]] for b in range(0, len(d), Batch)]

board_encoder = board_encode(D=512).to(d)
move_embedder = move_encode(D=512).to(d)
scheduler = noising(T=1000).to(d)
dit = diffusion(D=512, num_blocks=24, num_heads=16, num_experts=32, top_k=3).to(d)

scaler = torch.amp.GradScaler()

optimizer = bnb.optim.AdamW8bit(
    list(board_encoder.parameters()) +
    list(move_embedder.parameters()) +
    list(dit.parameters()), 
    lr=3e-5
)

save_dir = 'params'
num_epochs = 10
for epoch in range(num_epochs):
    dataloader = create_batch()
    for batch in dataloader:
        b_curr = len(batch)
        C = torch.stack([board_encoder(b['s']) for b in batch])
        x_0 = torch.stack([move_embedder(b['future']) for b in batch])
        
        t = scheduler.sample_time(b_curr)
        x_t, noise = scheduler.add_noise(x_0, t)
        
        branch_ids = torch.randint(0, 4, (b_curr,), device= d)
        sqrt_ab = scheduler.sqrt_alpha_bar[t].view(-1,1,1)
        sqrt_1mab = scheduler.one_alpha_bar[t].view(-1,1,1)
        v_target = sqrt_ab * noise - sqrt_1mab*x_0

        
        
        with torch.amp.autocast(device_type='cuda', dtype=torch.float16):
            v_pred, balance_loss = dit(x_t, t, C, branch_ids)
            diff_loss = F.mse_loss(v_pred, v_target)
            total_loss = diff_loss + 0.01*balance_loss
            
        print(f'loss : {total_loss}, epoch : {epoch}')

        scaler.scale(total_loss).backward()
        scaler.step(optimizer)
        scaler.update()
        
        if (epoch + 1) % 2 == 0:
            checkpoint_path = os.path.join(save_dir, f'{epoch+1}.pt')

            torch.save({
                'epoch': epoch + 1,
                'dit_state_dict': dit.state_dict(),
                'board_encoder_state_dict': board_encoder.state_dict(),
                'move_embedder_state_dict': move_embedder.state_dict(),
                'optimizer_state_dict': optimizer.state_dict(),
                'scaler_state_dict': scaler.state_dict(),
                'loss': total_loss.item(),
            }, checkpoint_path)

        

        

