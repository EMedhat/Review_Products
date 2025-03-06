from flask import Flask, render_template, request, redirect, url_for, flash
from flask_sqlalchemy import SQLAlchemy
import os

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///products.db'
app.config['UPLOAD_FOLDER'] = 'static/uploads'
app.config['SECRET_KEY'] = 'your_secret_key'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False

db = SQLAlchemy(app)

# Create upload directory if it doesn't exist
os.makedirs(app.config['UPLOAD_FOLDER'], exist_ok=True)

class Product(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(100), nullable=False)
    image = db.Column(db.String(100), nullable=False)
    reviews = db.relationship('Review', backref='product', lazy=True)

class Review(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    text = db.Column(db.Text, nullable=False)
    rating = db.Column(db.Integer, nullable=False)
    product_id = db.Column(db.Integer, db.ForeignKey('product.id'), nullable=False)

@app.route('/')
def index():
    products = Product.query.all()
    return render_template('index.html', products=products)

@app.route('/add_product', methods=['GET', 'POST'])
def add_product():
    if request.method == 'POST':
        name = request.form['name']
        image = request.files['image']
        
        if name and image:
            filename = image.filename
            image.save(os.path.join(app.config['UPLOAD_FOLDER'], filename))
            
            new_product = Product(name=name, image=filename)
            db.session.add(new_product)
            db.session.commit()
            
            flash('Product added successfully!', 'success')
            return redirect(url_for('index'))
    
    return render_template('add_product.html')

@app.route('/product/<int:product_id>', methods=['GET', 'POST'])
def view_product(product_id):
    product = Product.query.get_or_404(product_id)
    
    if request.method == 'POST':
        text = request.form['review']
        rating = int(request.form['rating'])
        
        new_review = Review(text=text, rating=rating, product_id=product.id)
        db.session.add(new_review)
        db.session.commit()
        
        flash('Review added successfully!', 'success')
        return redirect(url_for('view_product', product_id=product.id))
    
    return render_template('product.html', product=product)

if __name__ == '__main__':
    with app.app_context():
        db.create_all()
    app.run(debug=True)